#include "cserver.h"
#include "csession.h"
#include "servicepool.h"
#include <iostream>

CServer::CServer(boost::asio::io_context &ioc, short port)
    : m_ioc(ioc)
    , m_port(port)
    , m_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
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

    std::cout << "Accepted connection from: "
                  << new_session->GetSocket().remote_endpoint().address().to_string()
                  << ":" << new_session->GetSocket().remote_endpoint().port() << std::endl;

    new_session->Start();
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));

    StartAccept();
}
