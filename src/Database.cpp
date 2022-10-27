#include "Database.h"
#include <iostream>
 
using namespace std;
 
Database::Database()
{
	connection = mysql_init(NULL); 
	if(connection == NULL)
	{
		std::cout << "error:" << mysql_error(connection);
		exit(1);			
	}
}
 
Database::~Database()
{
	if(connection != NULL)
	{
		mysql_close(connection); 
	}
}
 
int Database::InitDB(std::string host, std::string user, std::string password, std::string db_name)
{
    char value = 1;
    mysql_options(connection,MYSQL_OPT_RECONNECT,(char*)&value);
        
	connection = mysql_real_connect(connection, host.c_str(), user.c_str(), password.c_str(), db_name.c_str(), 0, NULL, 0); // 建立数据库连接
	if(connection == NULL)
	{
		std::cout << "error:" << mysql_error(connection);
		exit(1);			
	}
	return 0;
}
 
int Database::SelectUsername(std::string username)
{
	std::string sql;
	sql = "select username from player where username = '" + username +"';";
    int flag;
    if(mysql_query(connection, sql.c_str()))
    {
		std::cout << "Query Error:" << mysql_error(connection);
        exit(1);
    }
    else
    {
        result = mysql_store_result(connection); 
        if (result->row_count > 0) flag=1;
        else flag=0;
        mysql_free_result(result);
    }
    return flag;
}
 
int Database::ChechPassword(std::string username,std::string password,int& playerid){
    std::string sql;
	sql = "select * from player where username = '" + username +"';";
    int flag;
    if(mysql_query(connection, sql.c_str()))
    {
		std::cout << "Query Error:" << mysql_error(connection);
        exit(1);
    }
    else
    {
        result = mysql_store_result(connection); 
        row = mysql_fetch_row(result);
		if (result->row_count > 0) {
			if (row[2] == password) {
				flag=1;
				playerid = atoi(row[0]);
			}
        	else flag=0;
		}
		else flag=0;
        mysql_free_result(result);
    }
    return flag;
}
int Database::InsertSQL(std::string username,std::string password)
{

	string sql = "insert into player(username,password) values ( '" + username +"','"+ password +"');";
	if(mysql_query(connection, sql.c_str()))
	{
		std::cout << "Query Error:" << mysql_error(connection);
		exit(1);
	}
	return 0;
}
 
int Database::DeleteSQL()
{
	return 0;
}
 
int Database::UpdateSQL()
{
	return 0;
}