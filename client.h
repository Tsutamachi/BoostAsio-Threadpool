#pragma once
#include "file.h"
#include <boost/asio.hpp>
#include <memory>
#include <vector>

//需要添加一个函数，检测到对端socket.close()后自己socket.close()，然后重新连接该Server
class Client
{
public:
    Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket& socket, short port);
    void HandleSessionClose(); //处理关闭的Session
    void AddFileToSend();
    FileToSend FindFileToSend(short fileid);

private:
    std::string m_ServerIp;
    short m_port;
    boost::asio::io_context& m_Ioc;
    boost::asio::ip::tcp::socket& m_Socket;
    std::shared_ptr<CSession> m_Session;

    std::vector<std::unique_ptr<FileToSend>> m_FilesToSend;
    //TempFile为了解决在没有FileId前，不能对应到m_FilesToSend进行存储的问题。
    std::unique_ptr<FileToSend> m_TempFile;
};
