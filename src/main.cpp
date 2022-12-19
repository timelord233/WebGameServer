#include <memory>
#include <vector>
#include <random>
#include "App.h"
#include "nlohmann/json.hpp"
#include "Database.h"

#define WEBSOCKET_CONNECT_MSG   0
#define DRAW_MSG                1
#define CHAT_MSG                2
#define UPDATE_PLAYER_INFO_MSG  3
#define PLAYER_READY_MSG        4
#define GAME_START_MSG			5
#define GAME_FINISHED_MSG		6

using json = nlohmann::json;

/* ws->getUserData returns one of these */
struct PerSocketData 
{
    int playerId;
    bool isReady = false;
    int score = 0;
    uWS::WebSocket<false, true> * ws = nullptr;
    bool operator == (const PerSocketData &data)
    {
        return data.playerId == playerId;
    }
    bool operator == (const int& id)
    {
        return id == playerId;
    }
};

void to_json(json& j, const PerSocketData& p) {
    j = json{{"PlayerId", p.playerId}, {"IsReady", p.isReady}, {"Score", p.score}};
}

std::shared_ptr<Database> database;
std::vector<PerSocketData*> Clients;
int Round = 0;
std::string CurrentWord;
std::vector<std::string> WordList = {"apple","pear","desk","angry","boat","happy"};

void InitDataBase()
{
    database = std::make_shared<Database>();
    database->InitDB("localhost","root","12345687","test_db");
}

void CORS(auto *res){
    res->writeHeader("Access-Control-Allow-Origin", "*");
    res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res->writeHeader("Access-Control-Allow-Headers", "origin, content-type, accept, x-requested-with");
    res->writeHeader("Access-Control-Max-Age", "3600");
}

void Register(auto *res, std::string username, std::string password)
{
    std::cout<<"Register: "<<username<<" "<<password<<std::endl;
    if (database->SelectUsername(username))
    {
        json data = R"({"IsSuccess":false})";
        res->end(data.dump());
    }
    else
    {
        database->InsertSQL(username,password);
        json data = R"({"IsSuccess":true})";
        res->end(data.dump());
    }
}

void Login(auto *res, std::string username, std::string password)
{
    std::cout<<"Login: "<<username<<" "<<password<<std::endl;
    int playerId;
    if (database->SelectUsername(username) && database->ChechPassword(username,password,playerId))
    {
        json data;
        data["IsSuccess"] = true;
        data["PlayerId"] = playerId;
        res->end(data.dump());
        std::cout<<"PlayerId: "<<playerId<<std::endl;
    }
    else
    {
        json data = R"({"IsSuccess":false})";
        res->end(data.dump());
    }
}

void UpdatePlayerInfo(auto *ws)
{
    json data;
    data["MessageType"] = UPDATE_PLAYER_INFO_MSG;
    data["PlayerInfoList"] = json::array();
    for (auto client : Clients)
    {
        data["PlayerInfoList"].push_back(*client);
    }
    ws->send(data.dump(),uWS::OpCode::TEXT,true);
    ws->publish("Boardcast",data.dump(),uWS::OpCode::TEXT);
}

void JoinRoom(auto *ws,int playerId)
{
    PerSocketData* data = (PerSocketData*)ws->getUserData();
    data->playerId = playerId;
    data->ws = ws;
    Clients.push_back(data);
    UpdatePlayerInfo(ws);
}

void StartGame()
{
    json guessData;
    guessData["MessageType"] = GAME_START_MSG;
    guessData["Round"] = ++Round;
    json drawData = guessData;

    std::default_random_engine e(time(0));
    std::uniform_int_distribution<int> u(0, Clients.size() - 1);
    std::uniform_int_distribution<int> uword(0, WordList.size() - 1);

    drawData["IsDrawer"] = true;
    CurrentWord = WordList[uword(e)];
    drawData["Word"] = CurrentWord;
    auto *ws = Clients[u(e)]->ws;
    ws->send(drawData.dump(),uWS::OpCode::TEXT,true);
    guessData["IsDrawer"] = false;
    ws->publish("Boardcast",guessData.dump(),uWS::OpCode::TEXT);
}

void SetReady(auto *ws,int playerId,bool isReady)
{
    PerSocketData* data = (PerSocketData*)ws->getUserData();
    data->isReady = isReady;
    UpdatePlayerInfo(ws);
    bool isAllReady = true;
    for (auto client : Clients)
    {
        if (client->isReady == false) 
            isAllReady = false;
    }
    if (isAllReady)
    {
        StartGame();
    }
}

void HandleChat(auto *ws,int playerId,std::string content)
{
    if (content == CurrentWord) 
    {
        PerSocketData* socketData = (PerSocketData*)ws->getUserData();
        socketData->score += 10;
        json data;
        data["MessageType"] = GAME_FINISHED_MSG;
        data["Winner"] = playerId;
        ws->send(data.dump(),uWS::OpCode::TEXT,true);
        ws->publish("Boardcast",data.dump(),uWS::OpCode::TEXT);  
        for (auto client : Clients)
        {
            client->isReady = false;
        }
        UpdatePlayerInfo(ws);
    }
}

void HandleDraw(auto *ws,std::string_view message)
{
    ws->publish("Boardcast",message,uWS::OpCode::TEXT);
}

void OnMessage(auto *ws, std::string_view message, uWS::OpCode opCode)
{
    std::cout<<"Message: "<<message<<std::endl;
    json data = json::parse(message);
    switch ((int)data["MessageType"])
    {
        case WEBSOCKET_CONNECT_MSG:
        {
            JoinRoom(ws,data["PlayerId"]);
            break;
        }
        case PLAYER_READY_MSG:
        {
            SetReady(ws,data["PlayerId"],data["IsReady"]);
            break;
        }
        case DRAW_MSG:
        {
            HandleDraw(ws,message);
            break;
        }
        case CHAT_MSG:
        {
            HandleChat(ws,data["PlayerId"],data["Content"]);
            break;
        }
        default:
            break;
      }
}



int main() 
{
    InitDataBase();

    uWS::App *app = new uWS::App();
    app->get("/*", [](auto *res, auto *req) {
        CORS(res);
	    res->end("Hello world");
    });
    app->options("/*", [](auto *res, auto *req){
        CORS(res);
        res->end("Hello world");
    });
    app->post("/register", [](auto *res, auto *req){
        CORS(res);
        res->onData([res, bodyBuffer = (std::string *)nullptr](std::string_view chunk, bool isLast) mutable{
            if (isLast) {
				if (bodyBuffer) {
					bodyBuffer->append(chunk);
                    json data = json::parse(*bodyBuffer);
                    Register(res,data["email"],data["pass"]);
					delete bodyBuffer;
				} else {
                    json data = json::parse(chunk);
                    Register(res,data["email"],data["pass"]);
				}
			} else {
				if (!bodyBuffer) {
					bodyBuffer = new std::string;
				}
				bodyBuffer->append(chunk);
			}
        });
		res->onAborted([]() {
			printf("Stream was aborted!\n");
		});
	});
    app->post("/login", [](auto *res, auto *req){
        CORS(res);
        res->onData([res, bodyBuffer = (std::string *)nullptr](std::string_view chunk, bool isLast) mutable{
            if (isLast) {
				if (bodyBuffer) {
					bodyBuffer->append(chunk);
					json data = json::parse(*bodyBuffer);
                    Login(res,data["email"],data["pass"]);
					delete bodyBuffer;
				} else {
					json data = json::parse(chunk);
                    Login(res,data["email"],data["pass"]);				
                }
			} else {
				if (!bodyBuffer) {
					bodyBuffer = new std::string;
				}
				bodyBuffer->append(chunk);
			}
        });
		res->onAborted([]() {
			printf("Stream was aborted!\n");
		});
	});
    app->ws<PerSocketData>("/Game", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 600,
        .maxBackpressure = 1 * 1024 * 1024,
        /* Handlers */
        .upgrade = nullptr,
        .open = [](auto *ws) {
            PerSocketData* data = (PerSocketData*)ws->getUserData();
            std::cout<<"New Connection! "<<std::endl;
            ws->subscribe("Boardcast");
        },
        .message = [&app](auto *ws, std::string_view message, uWS::OpCode opCode) {
            OnMessage(ws,message,opCode);
        },
        .drain = [](auto *ws) {
            
        },
        .ping = [](auto *ws) {
            
        },
        .pong = [](auto *ws) {
            
        },
        .close = [](auto *ws, int code, std::string_view message) {
            std::cout<<"Clients Num:"<<Clients.size()<<" "<<message<<std::endl;
            PerSocketData* sdata = (PerSocketData*)ws->getUserData();
            Clients.erase(std::remove_if(Clients.begin(),Clients.end(),
                [sdata](PerSocketData* data){
                    return data->playerId == sdata->playerId;
                }),Clients.end());
            UpdatePlayerInfo(ws);
        }
    });
    app->listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    });
    app->run();
    
    delete app;

    uWS::Loop::get()->free();
}





/*
ws://207.148.104.9:9001/Game

{
    "MessageType":0,
    "PlayerId":1
}

{
    "MessageType":4,
    "PlayerId":1,
    "IsReady":true
}

{
    "MessageType":2,
    "PlayerId":1,
    "Content":""
}

*/