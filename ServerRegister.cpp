#include "ServerRegister.h"
#include <iomanip>
#include <iostream>
#include <regex>
#include <vector>
using std::endl;
using std::vector;
ServerRegister::ServerRegister(QObject *parent)
{
    Connectdb *db = new Connectdb();
    conn=db->conn;
}
int ServerRegister::registerServer(QString names, QString passwords, QString emails)
{
    std::string name=names.toStdString();
    std::string email = emails.toStdString();
     std::string password = passwords.toStdString();

    string query = "INSERT INTO server ( username, "
                   "email, password) VALUES ('"
                   +  name + "','" + email + "','"
                   + password + "')";
     if(!isUniqeName(name)){
        return 2;
    }
    else if(!isUniqeEmail(email)){
        return 1;
    }

    else if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "mysql_query failed: " << mysql_error(conn) << endl;
        return -1;
    }

    else {
        std::cout << "Insert successful." << endl;
        return 0;
    }
}

// string ServerRegister::generateAccount()
// {
//     // string scount = "";
//     // while (true) {
//     //     scount.clear();
//     //     srand(time(0));
//     //     for (int i = 0; i < 10; i++) {
//     //         // 生成0-9之间的随机整数
//     //         int r = rand() % 10;
//     //         char c = r + '0';
//     //         scount += c;
//     //     }
//     //     if (isUniqeAccount(scount)) {
//     //         break;
//     //     }
//     // }
//     int scount = 1000000001;
//     std::string str = generateAccountWithLeadingZeros(scount, 10);
//     while (!isUniqeAccount(str)) {
//         scount++;
//         str = generateAccountWithLeadingZeros(scount, 10);
//     }

//     return str; // 返回生成的新账号
// }
bool ServerRegister::isUniqeName(string name)
{
    if (conn == nullptr) {
        std::cerr << "数据库连接为空" << std::endl;
        return false;
    }

    // 创建足够大的缓冲区（通常为源字符串长度的2倍 + 1）
    std::vector<char> escapedBuffer(name.length() * 2 + 1);

    // 调用mysql_real_escape_string（传入目标缓冲区）
    unsigned long escapedLength = mysql_real_escape_string(
        conn,                  // 数据库连接
        escapedBuffer.data(),  // 目标缓冲区
        name.c_str(),          // 源字符串
        name.length()          // 源字符串长度
    );

    if (escapedLength == 0) {
        std::cerr << "转义失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 构建安全的SQL查询
    std::string query = "SELECT 1 FROM server WHERE username = '";
    query.append(escapedBuffer.data(), escapedLength); // 添加转义后的字符串
    query += "' LIMIT 1";

    // 执行查询
    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "查询失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 获取结果集
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        std::cerr << "获取结果失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 检查是否存在记录
    bool exists = (mysql_num_rows(res) > 0);

    // 释放资源
    mysql_free_result(res);
    return !exists;
}


bool ServerRegister::isUniqeEmail(string email)
{
    if (conn == nullptr) {
        std::cerr << "数据库连接为空" << std::endl;
        return false;
    }

    // 创建足够大的缓冲区（通常为源字符串长度的2倍 + 1）
    std::vector<char> escapedBuffer(email.length() * 2 + 1);

    // 调用mysql_real_escape_string（传入目标缓冲区）
    unsigned long escapedLength = mysql_real_escape_string(
        conn,                  // 数据库连接
        escapedBuffer.data(),  // 目标缓冲区
        email.c_str(),          // 源字符串
        email.length()          // 源字符串长度
    );

    if (escapedLength == 0) {
        std::cerr << "转义失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 构建安全的SQL查询
    std::string query = "SELECT 1 FROM server WHERE email = '";
    query.append(escapedBuffer.data(), escapedLength); // 添加转义后的字符串
    query += "' LIMIT 1";

    // 执行查询
    if (mysql_query(conn, query.c_str()) != 0) {
        std::cerr << "查询失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 获取结果集
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        std::cerr << "获取结果失败: " << mysql_error(conn) << std::endl;
        return false;
    }

    // 检查是否存在记录
    bool exists = (mysql_num_rows(res) > 0);

    // 释放资源
    mysql_free_result(res);
    return !exists;
}
// string ServerRegister::generateAccountWithLeadingZeros(int scount, int width)
// {
//     std::ostringstream ss;
//     ss << std::setw(width) << std::setfill('0') << scount;
//     return ss.str();
// }
// bool ServerRegister::isUniqeName(string name)
// {
//     MYSQL_RES *res = nullptr;
//     MYSQL_ROW row;
//     vector<string> names;
//     if (mysql_query(conn, "SELECT servername FROM server") != 0) {
//         std::cerr << "mysql_query failed: " << mysql_error(conn) << std::endl;
//         return "";
//     }
//     res = mysql_use_result(conn);
//     if (res == nullptr) {
//         std::cerr << "mysql_use_result failed: " << mysql_error(conn) << std::endl;
//         return "";
//     }
//     while ((row = mysql_fetch_row(res)) != NULL) {
//         if (row[0] != nullptr) {
//             names.push_back(row[0]);
//         }
//     }
//     mysql_free_result(res);
//     for (int i = 0; i < names.size(); i++) {
//         if (names[i] == name) {
//             return false;
//         }
//     }
//     return true;
// }
// bool ServerRegister::isValidPassword(string password)
// {
//     // 正则表达式，要求至少一个大写字母、一个小写字母和一个特殊字符，且长度至少为8
//     std::regex passwordPattern(R"(^(?=.*[a-z])(?=.*[A-Z]).{8,}$)");

//     // 使用正则表达式匹配密码
//     if (std::regex_match(password, passwordPattern)) {
//         return true;
//     } else {
//         return false;
//     }
// }
// bool ServerRegister::isValidemail(string email)
// {
//     std::regex emailPattern(R"((?:[a-zA-Z0-9._%±]+)@(?:[a-zA-Z0-9.-]+.[a-zA-Z]{2,}))");

//     // 使用正则表达式匹配邮箱格式是否正确
//     if (std::regex_match(email, emailPattern)) {
//         return true;
//     } else {
//         return false;
//     }
// }
