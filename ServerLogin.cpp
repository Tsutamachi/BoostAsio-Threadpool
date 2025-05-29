// #include "ServerLogin.h"
#include <iostream>
using std::cout;
ServerLogin::ServerLogin(QObject *parent)
{
    Connectdb *db = new Connectdb();
    conn=db->conn;
}
int ServerLogin::serverLogin(QString emails, QString passwords)
{
    std::string email = emails.toStdString();
    std::string password = passwords.toStdString();
    if (!accountIsExist(email, password)) {
        std::cout << "用户账号不存在请检查用户名";
        return 1;
    }
    cout << "用户存在";
    if (!passwordIsCorrect(email, password)) {
        std::cout << "用户密码错误";
        return 2;
    }

    std::cout << "登陆成功！";
    return 0;
}
map<std::string, std::string> ServerLogin::queryemailAndPassword()
{
    std::map<std::string, std::string> accountPasswordPairs;
    MYSQL_RES *res;
    MYSQL_ROW row;
    const char *query = "SELECT email,password  FROM server";
    if (mysql_query(conn, query)) {
        std::cerr << "mysql_query failed: " << mysql_error(conn) << std::endl;
        return accountPasswordPairs;
    }
    res = mysql_store_result(conn);
    if (res == NULL) {
        std::cerr << "mysql_store_result failed: " << mysql_error(conn) << std::endl;
        return accountPasswordPairs;
    }
    while ((row = mysql_fetch_row(res)) != NULL) {
        std::string email(row[0]);
        std::string password(row[1]);
        accountPasswordPairs[email] = password;
    }
    mysql_free_result(res);
    return accountPasswordPairs;
}
bool ServerLogin::accountIsExist(string email, string password)
{
    std::map<std::string, std::string> accountPasswordPairs = queryemailAndPassword();
    auto it = accountPasswordPairs.find(email);
    if (it != accountPasswordPairs.end()) {
        return true;
    } else {
        return false;
    }
}
bool ServerLogin::passwordIsCorrect(string email, string password)
{
    std::map<std::string, std::string> accountPasswordPairs = queryemailAndPassword();
    auto it = accountPasswordPairs.find(email);
    if (it->second == password) {
        return true;
    } else {
        return false;
    }
}
