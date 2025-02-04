#pragma once
#include "client.h"
#include "defines.h"
#include "logicsystem.h"
#include "msgnode.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>

class CServer;
class MsgNode;
class Client;

class CSession : public std::enable_shared_from_this<CSession>
{
    friend class CServer;
    friend class MegNode;
    friend class RecevNode;
    friend class LogicSystem;
    friend class Client;

public:
    enum class Role { Server, Client };
    CSession(boost::asio::io_context& io_context,
             void* owner,
             Role role,
             boost::asio::ip::tcp::socket&& socket);
    boost::asio::ip::tcp::socket& GetSocket();
    std::string& GetUuid();
    void Start();
    void Send(const char* msg, short max_len, short msg_id);
    void SocketClose();
    std::shared_ptr<CSession> SharedSelf();
    short GetFileId();

    void Close();
    CServer* GetServerOwner() const
    {
        return (m_Role == Role::Server) ? static_cast<CServer*>(m_Owner) : nullptr;
    }

    Client* GetClientOwner() const
    {
        return (m_Role == Role::Client) ? static_cast<Client*>(m_Owner) : nullptr;
    }

    Role GetRole() const { return (m_Role == Role::Client) ? Role::Client : Role::Server; }

private:
    void HandleReadHead(const boost::system::error_code& error,
                        size_t bytes_transferred,
                        std::shared_ptr<CSession> shared_self);
    void HandleReadMeg(const boost::system::error_code& error,
                       size_t bytes_transferred,
                       std::shared_ptr<CSession> shared_self);
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    Role m_Role;   // 标识会话是 Server 还是 Client
    void* m_Owner; // 指向 CServer 或 Client 的指针（需类型安全转换）
    boost::asio::ip::tcp::socket m_Socket;
    boost::asio::io_context& m_IoContext;
    std::string m_Uuid;
    std::queue<std::shared_ptr<SendNode>> m_SendQue;
    std::mutex m_SendLock;
    bool _close;

    char m_Data[MAX_LENGTH];
    std::shared_ptr<MsgNode> m_RecevHeadNode;
    std::shared_ptr<RecevNode> m_RecevMsgNode;
    // std::array<bool, MAX_UPLOAD_NUM> m_FileIds;
    bool m_FileIds[MAX_UPLOAD_NUM]; //true代表可用，false代表正在被占用
};
