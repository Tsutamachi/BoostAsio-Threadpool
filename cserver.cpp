#include "cserver.h"
#include "csession.h"
#include <iostream>
// #include "AsioIOServicePool.h"
#include "servicepool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
    : m_ioc(ioc)
    // 上下文,端口地址接受p4夏的所有地址
    , m_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
        std::cout << "Server start success, listen on port : " << port << std::endl;
}

CServer::CServer(boost::asio::io_context& ioc, short port, std::string name)
    : m_ioc(ioc)
    , m_port(port)
    , m_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , m_ServerName(name)
{
    std::cout << "Server start success, listen on port : " << m_port << std::endl;
    StartAccept();
}

CServer::~CServer() {}
void CServer::ClearSession(std::string uuid)
{
    m_sessions.erase(uuid);
}

void CServer::StartAccept()
{
    auto &iocontext = ServicePool::GetInstance().GetService();
    boost::asio::ip::tcp::socket sock(iocontext);
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(iocontext,
                                                                       this,
                                                                       CSession::Role::Server,
                                                                       std::move(sock));
    m_acceptor
        .async_accept(new_session->GetSocket(),
                      std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::HandleAccept(std::shared_ptr<CSession> &new_session,
                           const boost::system::error_code &error)
{
    if (error) {
        std::cout << "session accept failed, error is " << error.what() << std::endl;
        return;
    }

    std::cout << "\nAccepted connection from: "
                  << new_session->GetSocket().remote_endpoint().address().to_string()
                  << ":" << new_session->GetSocket().remote_endpoint().port() << std::endl;

    new_session->Start();
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));

    StartAccept();
}

void CServer::Start()
{
    auto self = shared_from_this();
    // 交给线程池来管理
    auto &iocontext = ServicePool::GetInstance().GetService();
    boost::asio::ip::tcp::socket sock(iocontext);
    std::shared_ptr<CSession> new_con = std::make_shared<CSession>(iocontext,
                                                                   this,
                                                                   CSession::Role::Server,
                                                                   std::move(sock)
                                                                   );
    m_acceptor.async_accept(new_con->GetSocket(), [this,self,new_con](boost::beast::error_code ec) {
        try {
            // 如果出错放弃这个连接,继续监听其他连接
            if (ec) {
                self->Start();
                return;
            }
            // 创建新连接并且创建CSession类来管理
            new_con->Start();
            // 继续监听
            // std::make_shared<CSession>(std::move(self->_socket))->Start();
             m_sessions.insert(std::make_pair(new_con->GetUuid(), new_con));

            self->Start();
        } catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
    });
}
