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

public:
    Q_INVOKABLE int  serverLogin(QString account, QString password);

    ServerLogin(QObject *parent = nullptr);
    bool accountIsExist(string account, string password);
    bool passwordIsCorrect(string account, string password);

    map<std::string, std::string> queryemailAndPassword();

private:
    MYSQL *conn;
};
