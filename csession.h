#pragma once
#include "client.h"
#include "defines.h"
#include "logicsystem.h"
#include "msgnode.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <boost/beast.hpp>

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
    friend class FileToReceve;

public:
    enum class Role { Server, Client };
    CSession(boost::asio::io_context& io_context,
             void* owner,
             Role role,
             boost::asio::ip::tcp::socket&& socket);
    ~CSession();

    boost::asio::ip::tcp::socket& GetSocket();

    std::string& GetUuid();

    void Start();

    void Send(const char* msg, short max_len, short msg_id);

    void SendHttp(std::string& msg); //参数暂定

    void SocketClose();

    std::shared_ptr<CSession> SharedSelf();

    short GetFileId();

    void Close();

    CServer* GetServerOwner() const;

    Client* GetClientOwner() const;

    Role GetRole() const;

private:
    std::string get_Http_FileName(std::string HttpBuffer);

    void HandleReadHead(const boost::system::error_code& error,
                        size_t bytes_transferred,
                        std::shared_ptr<CSession> shared_self);


    void StartHttpProcessing(std::shared_ptr<CSession> self);

    void HandleMyProtocol(const boost::system::error_code& error,
                          size_t bytes_transferred,
                          std::shared_ptr<CSession> shared_self);

    void HandleHttpProtocol(const boost::system::error_code& error,
                            size_t bytes_transferred,
                            std::shared_ptr<CSession> shared_self);

    void WriteResponse(std::shared_ptr<CSession> shared_self);

    void HandleReadMeg(const boost::system::error_code& error,
                       size_t bytes_transferred,
                       std::shared_ptr<CSession> shared_self);

    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    void PreParseGetParam();

    Role m_Role;   // 标识会话是 Server 还是 Client
    void* m_Owner; // 指向 CServer 或 Client 的指针（需类型安全转换）
    boost::asio::ip::tcp::socket m_Socket;
    boost::asio::io_context& m_IoContext;
    std::string m_Uuid;
    std::queue<std::shared_ptr<SendNode>> m_SendQue;
    std::mutex m_SendLock;
    std::mutex m_IdLock;

    //状态值
    bool _close;

    char m_Data[MAX_LENGTH];
    std::shared_ptr<MsgNode> m_RecevHeadNode;
    std::shared_ptr<RecevNode> m_RecevMsgNode;
    bool m_FileIds[MAX_UPLOAD_NUM]; //true代表可用，false代表正在被占用
    // std::atomic<short> m_NextFileId{0};

    //以下是Http相关的成员
    boost::beast::flat_buffer m_http_buffer;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::dynamic_body>> m_http_parser;
    boost::beast::http::request<boost::beast::http::dynamic_body> m_http_request;
    boost::beast::http::response<boost::beast::http::dynamic_body> m_http_response;
    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;
    // The timer for putting a deadline on connection processing.
    boost::asio::steady_timer deadline_{
        m_Socket.get_executor(), std::chrono::seconds(60) };
};
