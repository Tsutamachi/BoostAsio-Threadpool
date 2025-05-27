#pragma once
#include <mysql.h>
#include <stdio.h>
class Connectdb
{
private:
    const char *server = "localhost";
    const char *user = "root";
    const char *password = "root";
    const char *database = "home";
public:
    MYSQL *conn;
    Connectdb();
    void dbclosed();
};
