#pragma once
#include "csession.h"
#include "file.h"
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

//需要添加一个函数，检测到对端socket.close()后自己socket.close()，然后重新连接该Server?
class CSession;
class Client
{
    friend class CSession;
    friend class LogicSystem;

public:
    Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket& socket, short port);
    void HandleSessionClose(); //处理关闭的Session
    void AddFileToSend(std::unique_ptr<FileToSend> tempfile,
                       short fileid);                          //将TempFIle加入FilesToSend
    std::unique_ptr<FileToSend>& FindFileToSend(short fileid); //通过Fileid查询File

    void Test1();
    void EchoTest();        //网络：发送测试
    void RequestUpload();   //网络：发出上传文件到Server的请求
    void RequestDownload(); //网络：发出从Server中下载文件的请求

private:
    std::string m_ServerIp;
    short m_port;
    boost::asio::io_context& m_Ioc;
    boost::asio::ip::tcp::socket m_Socket;
    std::shared_ptr<CSession> m_Session;
    std::vector<std::thread> vec_threads;

    std::vector<std::unique_ptr<FileToSend>> m_FilesToSend;
    //TempFile为了解决在没有FileId前，不能对应到m_FilesToSend进行存储的问题。
    std::unique_ptr<FileToSend> m_TempFile;
};
