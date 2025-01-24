#pragma once
#include "defines.h"
#include "logicsystem.h"
#include "msgnode.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>

class CServer;
class MsgNode;
class CSession : public std::enable_shared_from_this<CSession>
{
    friend class CServer;
    friend class MegNode;
    friend class RecevNode;
    friend class LogicSystem;

public:
    CSession(boost::asio::io_context& io_context, CServer* server);
    boost::asio::ip::tcp::socket& GetSocket();
    std::string& GetUuid();
    void Start();
    void Send(const char* msg, short max_len, short msg_id);
    void SocketClose();
    std::shared_ptr<CSession> SharedSelf();

private:
    void HandleReadHead(const boost::system::error_code& error,
                        size_t bytes_transferred,
                        std::shared_ptr<CSession> shared_self);
    void HandleReadMeg(const boost::system::error_code& error,
                       size_t bytes_transferred,
                       std::shared_ptr<CSession> shared_self);
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    boost::asio::ip::tcp::socket m_Socket;
    boost::asio::io_context& m_IoContext;
    CServer* m_Server;
    std::string m_Uuid;
    std::queue<std::shared_ptr<MsgNode>> m_SendQue;
    std::mutex m_SendLock;
    bool _close;

    char m_Data[MAX_LENGTH];
    std::shared_ptr<MsgNode> m_RecevHeadNode;
    std::shared_ptr<RecevNode> m_RecevMsgNode;
};
