#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>

class CSession;
class CServer
{
    friend class CSession;

public:
    CServer(boost::asio::io_context& ioc, short port);
    void ClearSession(std::string uuid);

private:
    void StartAccept();
    void HandleAccept(std::shared_ptr<CSession>& new_session,
                      const boost::system::error_code& error);

    boost::asio::io_context& m_ioc;
    short m_port;
    std::mutex m_Mutex;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<CSession>> m_sessions;
};
