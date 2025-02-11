#pragma once
#include "Singleton.h"
#include "defines.h"
#include "file.h"
#include "logicnode.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

typedef std::function<void(std::shared_ptr<CSession> session, const std::string &msg_data)>
    FunCallBack;

class CSession;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class CSession;
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();
    void PostMesgToQue(std::shared_ptr<LogicNode> node);

private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks(); //注册：消息id <-> 回调函数

    //发出的请求：
    //CLient->Server:上传文件，下载文件
    //RecevServer->SharedServer：下载文件
    //需要写的请求处理包：上传文件（仅Client用），下载文件（可通用）

    //MsgId:Test                处理 回显测试
    void ClientSendTest(std::shared_ptr<CSession> session, const std::string &msg_data);
    //MsgId:Echo                接受 回显
    void ServerSendTest(std::shared_ptr<CSession> session, const std::string &msg_data);
    //MsgId:Back
    void ClientReturn(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileUploadRequest   处理 上传文件的请求
    void RequestUpload(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileDownloadRequest 处理 下载文件的请求
    void RequestDownload(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:ReturnFileId        回应 上传文件的请求--同意
    void HandleUploadRequest(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:RejectUpload        回应 上传文件的请求--拒绝
    void HandleRejectUpload(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileDataBag         回应 开始上传文件
    void HandleFileUpload(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileDataBag         回应 接受文件
    void HandleData(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileComplete        回应 接受最后一个包，并进行缺包检测
    void ServerHandleFinalBag(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:ServerRecevFinal    完成 确认Server接收到Final包
    void ClientHandleFinalBag(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:TellLostBag         回应 Client向Server发送缺包
    void HandleReTransmit(std::shared_ptr<CSession> session, const std::string &msg_data);

    //MsgId:FileComplete        完成 确认Upload功能
    void FinishUpload(std::shared_ptr<CSession> session, const std::string &msg_data);

    std::thread m_WorkerThread;
    std::queue<std::shared_ptr<LogicNode>> m_MegQue;
    std::mutex m_Mutex;
    std::map<short, FunCallBack> m_ClientFunCallBacks;
    std::map<short, FunCallBack> m_ServerFunCallBacks;
    std::condition_variable m_Consume; //能在某个条件成立之前 阻塞线程 并在条件成立时 唤醒线程的机制
    bool m_stop;

};
