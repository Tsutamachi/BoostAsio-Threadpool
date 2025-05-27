#pragma once
#include "csession.h"
#include "file.h"
#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

//需要添加一个函数，检测到对端socket.close()后自己socket.close()，然后重新连接该Server?
class FileToSend;
class CSession;
class Client
{
    friend class CSession;
    friend class LogicSystem;

public:
    Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket socket, short port, bool net);
    Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket socket, short port, bool net,std::string host);
    ~Client();
    void Greating();
    void HandleSessionClose(); //处理关闭的Session
    void AddFileToSend(std::shared_ptr<FileToSend> tempfile,
                       short fileid);                         //将TempFIle加入FilesToSend
    std::shared_ptr<FileToSend> FindFileToSend(short fileid); //通过Fileid查询File
    void RemoveFile(short fileid);
    void Test1();
    void EchoTest();        //网络：发送测试
    void RequestUpload();   //网络：发出上传文件到Server的请求
    void RequestDownload(); //网络：发出从Server中下载文件的请求
    void SendTestMsg();

private:
    void DealSendQueue();


    bool m_IsInnerNet;
    std::string m_Host;

    std::string m_ServerIp;
    short m_port;
    boost::asio::io_context& m_Ioc;
    // boost::asio::ip::tcp::socket m_Socket;
    std::shared_ptr<CSession> m_Session;
    std::vector<std::thread> vec_threads;

    //TempFile为了解决在没有FileId前，不能对应到m_FilesToSend进行存储的问题。
    //File相关的两个成员都是在LogicSystem中创建的
    std::shared_ptr<FileToSend> m_TempFile;
    std::vector<std::shared_ptr<FileToSend>> m_FilesToSend;

    //传输文件的缓存队列--需要哪些成员？filepath
    //只要队列中有数据，就请求FIleId.获得的FileId分配给队列中第一个元素
    //if（FileId用完 && m_Bool:FileFinish）  m_CV.notify.once()
    //HandleFileUpload中把获得fileid的filepath pop出去
    std::queue<std::string> m_SendQueue;
    std::condition_variable m_CVSendQue; //唤醒Send线程？
    std::mutex m_Mutex;
    short m_NowSend;

    std::string m_LogedServerName; // 被登陆的Server的账号
    int m_ClientId;                // 此Client在Server数据库中分配的Id
    std::string m_ClientName;      // 自定义的账号

    // 用于断电功能的状态值
    bool is_Uploading; //Client正在上传文件到Server 用于析构函数中判断状态，是否进行断点记载
    bool is_Downloading; //Client正在下载文件到Server
    unsigned int m_CurrentSeq; //这里要记录每一个文件的最近文件号
    // 当前传输的文件路径在m_FilesToSend里找
};
