#pragma once
#include <map>
#include <mysql.h>
#include <string>
#include "Connectdb.h"
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
using std::map;
using std::string;
class ServerLogin:public QObject
{
    Q_OBJECT
    QML_ELEMENT
private:
    MYSQL *conn;

public:
    ServerLogin(QObject *parent = nullptr);
    Q_INVOKABLE int  serverLogin(QString account, QString password);
    map<std::string, std::string> queryemailAndPassword();
    bool accountIsExist(string account, string password);
    bool passwordIsCorrect(string account, string password);
};
