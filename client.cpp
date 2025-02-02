#include "client.h"
#include "csession.h"

Client::Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket& socket, short port)
    : m_Ioc(ioc)
    , m_Socket(socket)
    , m_port(port)
    , m_Session(ioc, this, CSession::Role::Client)
{
    std::cout << "Client start success, listen on port : " << m_port << std::endl;
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(m_Ioc, this);
}

void Client::HandleSessionClose()
{
    //在这里可以添加Client的资源回收和各种文件的持久化保存，如断点续传的包数

    m_Session.reset(); //清理Session资源
    m_Socket.close();  //清理Socket资源

    //重连socket
    bool flag = true;
    while (flag) {
        boost::asio::ip::tcp::endpoint remote_ep(address::from_string(m_ServerIp), m_port);
        boost::asio::ip::tcp::socket sock(m_Ioc);
        boost::system::error_code error = boost::asio::error::host_not_found;
        ;
        sock.connect(remote_ep, error);
        if (error) {
            std::cerr << "connect failed, code is " << error.value() << " error msg is "
                      << error.message() << std::endl;
            continue;
        }
        m_Socket = sock;

        flag = false;
    }
}

void Client::AddFileToSend() {}

FileToSend Client::FindFileToSend(short fileid)
{
    //从m_FilesToSend中通过fileid索引找到FileToSend
}
