#include <string>
#include <mysql/mysql.h>
 
class Database
{
public:
	Database();
	~Database();
	int InitDB(std::string host, std::string user, std::string password, std::string db_name);
	int SelectUsername(std::string username);
    int ChechPassword(std::string username,std::string password,int& playerid);
	int InsertSQL(std::string username,std::string password);
	int DeleteSQL();
	int UpdateSQL();
private:
	MYSQL *connection;
	MYSQL_RES *result;
	MYSQL_ROW row;
};