#include <memory>
#include "App.h"
#include "nlohmann/json.hpp"
#include "Database.h"

using json = nlohmann::json;

std::shared_ptr<Database> database;

void InitDataBase()
{
    database = std::make_shared<Database>();
    database->InitDB("localhost","root","12345687","test_db");
    database->InsertSQL("Test","test");
}

int main() 
{
    InitDataBase();
    /* ws->getUserData returns one of these */
    struct PerSocketData 
    {
        /* Fill with user data */
    };

    uWS::App().get("/*", [](auto *res, auto *req) {
	    res->end("Hello world!");
    }).post("/*", [](auto *res, auto *req){
        res->onData([res, bodyBuffer = (std::string *)nullptr](std::string_view chunk, bool isLast) mutable{
            if (isLast) {
				if (bodyBuffer) {
					bodyBuffer->append(chunk);
					res->end(*bodyBuffer);
					delete bodyBuffer;
				} else {
					res->end(chunk);
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
	}).ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024,
        /* Handlers */
        .upgrade = nullptr,
        .open = [](auto *ws) {
            /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
            std::cout<<"New Connection!"<<std::endl;
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            ws->send(message, opCode, true);
        },
        .drain = [](auto *ws) {
            /* Check ws->getBufferedAmount() here */
        },
        .ping = [](auto *ws) {
            /* Not implemented yet */
        },
        .pong = [](auto *ws) {
            /* Not implemented yet */
        },
        .close = [](auto *ws, int code, std::string_view message) {
            std::cout<<message<<std::endl;
        }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}
