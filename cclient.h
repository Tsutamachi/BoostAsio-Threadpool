#ifndef CCLIENT_H
#define CCLIENT_H

#include "HttpMgr.h"
#include <functional>
#include <map>
#include <nlohmann/json.hpp> // 添加头文件
#include <regex>
#include <string>
#include <QObject> // 添加 QObject 头文件
#include <QQmlEngine>
#include<QtQml/qqmlregistration.h>

using json = nlohmann::json; // 引入命名空间

class CClient:public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString useremail READ useremail WRITE setUseremail NOTIFY useremailChanged FINAL)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged FINAL)
public:
    CClient(QObject *parent = nullptr);
    void initHttpHandlers();
    std::map<ReqId, std::function<void(const json&)>> _handlers; // 修改类型
    void on_get_code_clicked(const std::string& aemail);
    void slot_reg_mod_finish(ReqId id, const std::string& res, ErrorCode err);
    void slot_login_mod_finish(ReqId id, const std::string& res, ErrorCode err);
    void on_get_register_clicked(string username,string email,string password,string confirm,string varifycode);
    Q_INVOKABLE void on_get_login_clicked();
    QString useremail() const;
    void setUseremail(QString &email);
    QString password() const;
    void setPassword(QString &email);
signals:
    void useremailChanged();
    void passwordChanged();
private:
    QString m_email;
    QString m_password;
};

#endif // CCLIENT_H
