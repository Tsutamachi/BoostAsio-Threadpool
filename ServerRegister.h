#pragma once
#include <mysql.h>
#include <string>
#include "Connectdb.h"
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
using std::string;

class ServerRegister:public QObject
{
    Q_OBJECT
    QML_ELEMENT
private:
    MYSQL *conn;

public:
    ServerRegister(QObject *parent = nullptr);
    Q_INVOKABLE int registerServer(QString name, QString password, QString email);
    bool isUniqeName(string name);
    bool isUniqeEmail(string name);
    // string generateAccount();
    // bool isUniqeAccount(string sccount);
    // bool isUniqeName(string name);
    // bool isValidPassword(string password);
    // bool isValidemail(string email);
    // string generateAccountWithLeadingZesros(int scount, int width);
};
