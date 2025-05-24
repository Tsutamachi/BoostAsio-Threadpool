#include "cclient.h"
#include "HttpMgr.h"
#include <iostream>
#include <nlohmann/json.hpp>

// 为了方便使用，引入 nlohmann/json 命名空间
using json = nlohmann::json;

CClient::CClient(QObject *parent)
{
    initHttpHandlers();
    // 将 Lambda 表达式作为回调函数注册到HttpMgr，用于接收请求结果，再调用slot_reg_mod_finish处理响应。s
}


void CClient::initHttpHandlers()
{
    // 注册获取验证码回包的逻辑请求处理器注册
    // 传入的是一个jsonobject对象
    _handlers[ReqId::ID_GET_VARIFY_CODE] = [this](const json& jsonobject) {
        int error = jsonobject["error"].get<int>();
        if (error != static_cast<int>(ErrorCode::SUCCESS)) {
            std::cout << "参数错误" << std::endl;
            return;
        }
        std::string email = jsonobject["email"].get<std::string>();
        std::cout << "验证码已经发送到邮箱,注意查收" << std::endl;
        std::cout << email << std::endl;
    };
    _handlers[ReqId::ID_REG_USER] = [this](const json& jsonobject) {
        int error = jsonobject["error"].get<int>();
        if (error != static_cast<int>(ErrorCode::SUCCESS)) {
            std::cout << "参数错误" << std::endl;
            return;
        }
        std::string email = jsonobject["email"].get<std::string>();
        std::cout << "用户注册成功" << std::endl;
        std::cout << email << std::endl;
    };
    _handlers[ReqId::ID_LOGIN_USER]=[this](const json&jsonobject){
        std::cout<<"正在验证登陆";
        int error =jsonobject["error"].get<int>();
        if (error != static_cast<int>(ErrorCode::SUCCESS)) {
            std::cout << "参数错误" << std::endl;
            return;
        }
        std::cout<<"用户登陆成功！";
    };
}

void CClient::slot_reg_mod_finish(ReqId id, const std::string& res, ErrorCode err)
{
    if (err != ErrorCode::SUCCESS) {
        std::cout << "请求无效，错误码: " << static_cast<int>(err) << std::endl;
        return;
    }
    std::cout << "服务器响应: " << res << std::endl;
    try {
        json jsonDoc = json::parse(res);
        _handlers[id](jsonDoc);
        return;
    } catch (const json::parse_error& e) {
        std::cout << "json解析失败: " << e.what() << std::endl;
    }
}

void CClient::slot_login_mod_finish(ReqId id, const string &res, ErrorCode err)
{

    if (err != ErrorCode::SUCCESS) {
        std::cout << "请求无效，错误码: " << static_cast<int>(err) << std::endl;
        return;
    }
    // 通过客户端管理连接服务器返回的信息爱显示服务器的响应
     std::cout << "服务器响应: " << res << std::endl;
     try {
        // _handlers通过这个来调用handler中的回调函数
         // 根据注册或者登陆的id
         // std::cout<<id<<endl;
         json jsonDoc = json::parse(res);
         _handlers[id](jsonDoc);
     } catch (const json::parse_error& e) {
         std::cout << "json解析失败: " << e.what() << std::endl;
     }
}

// 注册

void CClient::on_get_register_clicked(string username,string email,string password,string confirm,string varifycode)
{
    // 定义在完成后需要调用的回调函数
    auto callback = [this](ReqId id, const std::string& res, ErrorCode err) {
        this->slot_reg_mod_finish(id, res, static_cast<ErrorCode>(err));
    };
      HttpMgr::GetInstance()->setCallback(callback);
        json json_obj;
        json_obj["user"] = username;
        json_obj["email"] = email;
        json_obj["passwd"] = password;
        json_obj["confirm"] = confirm;
        json_obj["varifycode"] = varifycode;
        HttpMgr::GetInstance()->PostHttpReq("http://localhost:8080/user_register",
                     json_obj.dump(), ReqId::ID_REG_USER,Modules::REGISTERMOD);

}

void CClient::on_get_code_clicked(const std::string& aemail)
{
    // auto callback = [this](ReqId id, const std::string& res, ErrorCode err) {
    //     this->slot_login_mod_finish(id, res, static_cast<ErrorCode>(err));
    // };
    cout<<"正在执行验证码服务";
    std::regex regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    if (std::regex_match(aemail, regex)) {
        // 发送 http 请求获取验证码
        json json_obj;
        json_obj["email"] = aemail;
        HttpMgr::GetInstance()->PostHttpReq("http://localhost:8080/get_varifycode",
                                            json_obj.dump(),
                                            ReqId::ID_GET_VARIFY_CODE,
                                            Modules::REGISTERMOD);
    } else {
        // 提示邮箱不正确
        std::cout << "邮箱地址不正确" << std::endl;
    }
}
void CClient::on_get_login_clicked()
{
    std::cout<<"正在启动登陆函数";
    // 定义在完成后需要调用的回调函数
    auto callback = [this](ReqId id, const std::string& res, ErrorCode err) {
        this->slot_login_mod_finish(id, res, static_cast<ErrorCode>(err));
    };
      HttpMgr::GetInstance()->setCallback(callback);
        json json_obj;
        json_obj["email"] = m_email.toStdString();
        json_obj["passwd"] = m_password.toStdString();
        HttpMgr::GetInstance()->PostHttpReq("http://localhost:8080/user_login",
                     json_obj.dump(), ReqId::ID_LOGIN_USER,Modules::REGISTERMOD);

}

QString CClient::useremail() const
{
    return m_email;
}
QString CClient::password() const
{
    return m_password;
}

void CClient::setUseremail(QString &email)
{
    if(email !=m_email){
        m_email=email;
    }

}
void CClient::setPassword(QString &pas)
{
    if(pas !=m_password){
        m_password=pas;
    }

}
