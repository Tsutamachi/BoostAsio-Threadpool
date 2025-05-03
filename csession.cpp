#include "csession.h"
#include "cserver.h"
#include "defines.h"
#include "logicsystem.h"
#include "msgnode.h"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <sstream>
#include "filetransfer.h"

// using boost::asio::awaitable;
// using boost::asio::co_spawn;
// using boost::asio::detached;
// using boost::asio::use_awaitable;

// 通用构造函数实现
CSession::CSession(boost::asio::io_context &io_context,
                   void *owner,
                   Role role,
                   boost::asio::ip::tcp::socket &&socket)
    : m_Socket(std::move(socket))
    , m_IoContext(io_context)
    , m_Owner(owner)
    , m_Role(role)
    , _close(false)

{
    // 初始化 UUID 和 FileIds
    boost::uuids::uuid am_Uuid = boost::uuids::random_generator()();
    m_Uuid = boost::uuids::to_string(am_Uuid);
    m_RecevHeadNode = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    std::memset(m_FileIds, true, sizeof(m_FileIds));
    std::cout << "CSession Construct!" << std::endl;
}

CSession::~CSession()
{
    if (m_Socket.is_open()) {
        boost::system::error_code ec;
        m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_Socket.close(); // 确保套接字关闭
    }
    // delete m_Data;//不是new的，就不需要手动回收
}

boost::asio::ip::tcp::socket &CSession::GetSocket()
{
    return m_Socket;
}

std::string &CSession::GetUuid()
{
    return m_Uuid;
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
    return shared_from_this(); //不增加引用计数。只有在真正调用回调函数是，才会+1
}

short CSession::GetFileId()
{
    std::unique_lock<std::mutex> lock(m_IdLock);

    int fileid = -1; //默认为没有能用的fileid
    for (int i = 0; i < MAX_UPLOAD_NUM; i++) {
        if (m_FileIds[i]) {
            fileid = i;
            m_FileIds[i] = false;
            break;
        }
    }
    return fileid;
}

CServer *CSession::GetServerOwner() const
{
    return (m_Role == Role::Server) ? static_cast<CServer *>(m_Owner) : nullptr;
}

Client *CSession::GetClientOwner() const
{
    return (m_Role == Role::Client) ? static_cast<Client *>(m_Owner) : nullptr;
}

CSession::Role CSession::GetRole() const
{
    return (m_Role == Role::Client) ? Role::Client : Role::Server;
}

std::string CSession::get_Http_FileName(std::string HttpBuffer)
{
    size_t path_start = HttpBuffer.find(" ") + 1;
    size_t path_end = HttpBuffer.find(" ", path_start);
    return HttpBuffer.substr(path_start, path_end - path_start);
}

void CSession::Start()
{
    //Edition1--非协程
    std::memset(m_Data, 0, MAX_LENGTH);
    boost::asio::async_read(m_Socket,
                            boost::asio::buffer(m_Data, HEAD_TOTAL_LEN),
                            std::bind(&CSession::HandleReadHead,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      SharedSelf()));
}

void CSession::SocketClose()
{
    m_Socket.close();
}

void CSession::HandleReadHead(const boost::system::error_code &error,
                              size_t bytes_transferred,
                              std::shared_ptr<CSession> shared_self)
{
    try {
        if (error) {
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理
            }
            std::cout << "ReadHead fails:" << error.what() << std::endl;
        }

        // std::cout << std::endl << "消息头部：" << std::endl;
        if (bytes_transferred < HEAD_TOTAL_LEN) {
            std::cout << "Read head length error" << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            return;
        }

        //如果有特殊要求：遇到某个字符就停止copy,可以用memccpy（Linux函数）.但是memcpy性能更好
        memcpy(m_RecevHeadNode->m_Data, m_Data, HEAD_TOTAL_LEN);
        m_RecevHeadNode->m_CurLen += bytes_transferred;

        // 检测是否为 HTTP 请求（GET 或 POST）
        if (memcmp(m_RecevHeadNode->m_Data, "GET ", HEAD_TOTAL_LEN) == 0
            || memcmp(m_RecevHeadNode->m_Data, "POST", HEAD_TOTAL_LEN) == 0) {
            std::cout << "Detected HTTP protocol" << std::endl;

            // this->HandleHttpProtocol(error, bytes_transferred, SharedSelf());
            m_HttpBuffer.append(m_RecevHeadNode->m_Data, HEAD_TOTAL_LEN);
            boost::asio::async_read_until(m_Socket,
                                          boost::asio::dynamic_buffer(m_HttpBuffer),//动态缓冲默认续写而非覆盖
                                          "\r\n\r\n",
                                          std::bind(&CSession::HandleHttpProtocol,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    SharedSelf()));
        } else {
            std::cout << "Detected custom protocol" << std::endl;
            this->HandleMyProtocol(error, bytes_transferred, SharedSelf());
        }

    } catch (std::exception &e) {
        std::cerr << "HandleReadHead fails.Exception code is:" << e.what() << std::endl;
    }
}

void CSession::HandleMyProtocol(const boost::system::error_code &error,
                                size_t bytes_transferred,
                                std::shared_ptr<CSession> shared_self)
{
    short msg_id = 0;
    memcpy(&msg_id, m_RecevHeadNode->m_Data, HEAD_ID_LEN);
    msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
    // std::cout << "CSession--msg_id :" << msg_id << std::endl;
    if (msg_id > MSGID_MAX || msg_id < MSGID_MIN) {
        std::cout << "invalid data id is :" << msg_id << std::endl;
        if (auto server = GetServerOwner()) {
            server->ClearSession(m_Uuid); // Server 端清理
            SocketClose();
        } else if (auto client = GetClientOwner()) {
            client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
        }
        return;
    }

    short msg_len = 0;
    memcpy(&msg_len, m_RecevHeadNode->m_Data + HEAD_ID_LEN, HEAD_DATA_LEN);
    msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
    // std::cout << "CSession--msg_len_to_host :" << msg_len << std::endl << std::endl;
    if (msg_len > 4000) {
        std::cout << "invalid data length is :" << msg_len << std::endl;
        if (auto server = GetServerOwner()) {
            server->ClearSession(m_Uuid); // Server 端清理
            SocketClose();
        } else if (auto client = GetClientOwner()) {
            client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
        }
        return;
    }

    m_RecevHeadNode->Clear();
    m_RecevMsgNode = std::make_shared<RecevNode>(msg_len, msg_id);

    std::memset(m_Data, 0, HEAD_TOTAL_LEN);
    boost::asio::async_read(m_Socket,
                            boost::asio::buffer(m_Data, msg_len),
                            std::bind(&CSession::HandleReadMeg,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      SharedSelf()));
}

void CSession::HandleHttpProtocol(const boost::system::error_code &error,
                                  size_t bytes_transferred,
                                  std::shared_ptr<CSession> shared_self)
{
    try {
        if (error) {
            std::cerr << "HttpRequest_ReadLast fails:" << error.what() << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
        }

        std::cout << "Http请求头：" << std::endl;
        std::cout << m_HttpBuffer << std::endl;

        if(!memcmp(m_RecevHeadNode->m_Data, "GET ", HEAD_TOTAL_LEN))
        {
            //处理GET请求:找出GET请求的文件名，传输（做成一个通用函数）
            std::string request_path = get_Http_FileName(m_HttpBuffer);
            std::cout<<"FilePath: "<<request_path<<std::endl;

            //不允许查看父目录
            if (request_path.find("..") != std::string::npos) {
                // send_http_error(403, "Forbidden");
                return;
            }

            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
            boost::asio::async_write(m_Socket,
                              boost::asio::buffer(response));
                    // SendHttp(response);

            // FileTransfer::SendFile(this->m_Socket,request_path);
        }
        else
        {
            //POST的处理：找出length，接收数据后送给logicSystem做处理
            size_t header_end = m_HttpBuffer.find("\r\n\r\n"); //返回的索引是指向第一个\r的索引
            std::string headers = m_HttpBuffer.substr(0, header_end); //创造一个header副本
            std::istringstream stream(headers);
            std::string line;
            unsigned long msg_len = 0;

            //获取Msg部分的长度msg_len
            while (std::getline(stream, line)) {
                std::transform(line.begin(), line.end(), line.begin(), ::tolower); // 处理大小写问题
                if (line.find("content-length:") != std::string::npos) {           //找到了
                    // +2跳过“:”和“空格”
                    msg_len = std::stoul(line.substr(line.find(":") + 2)); //string to unsigned long;
                    break;
                }
            }

            if (msg_len) {
                m_RecevMsgNode = std::make_shared<RecevNode>(msg_len,
                                                             0); //msg_id在读取信息里重新设置

                std::memset(m_Data, 0, HEAD_TOTAL_LEN);
                boost::asio::async_read(m_Socket,
                                        boost::asio::buffer(m_Data, msg_len), //m_Data[2048]  2kb
                                        std::bind(&CSession::HandleHttpReadMeg,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  SharedSelf()));
            }
        }

    } catch (std::exception &e) {
        std::cerr << "HandleHttpReadMsg fails.\n  Exception code is:" << e.what() << std::endl;
    }
}

void CSession::HandleHttpReadMeg(const boost::system::error_code &error,
                                 size_t bytes_transferred,
                                 std::shared_ptr<CSession> shared_self)
{}

void CSession::HandleReadMeg(const boost::system::error_code &error,
                             size_t bytes_transferred,
                             std::shared_ptr<CSession> shared_self)
{
    try {
        if (error) {
            std::cerr << "ReadMsg fails:" << error.what() << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            //没有return
        }
        // std::cout << std::endl << "消息体:" << std::endl;
        memcpy(m_RecevMsgNode->m_Data, m_Data, m_RecevMsgNode->m_TotalLen);
        m_RecevMsgNode->m_CurLen += bytes_transferred;

        LogicSystem::GetInstance()->PostMesgToQue(
            make_shared<LogicNode>(shared_from_this(), m_RecevMsgNode));

        std::memset(m_Data, 0, HEAD_TOTAL_LEN);
        // m_RecevMsgNode->Clear();

        boost::asio::async_read(m_Socket,
                                boost::asio::buffer(m_Data, HEAD_TOTAL_LEN),
                                std::bind(&CSession::HandleReadHead,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          SharedSelf()));
    } catch (std::exception &e) {
        std::cerr << "HandleReadMsg fails.Exception code is:" << e.what() << std::endl;
    }
}

// std::string msg_data = root.toStyledString();
// Send(msg_data.data(), msg_data.size(), (short) root["id"].asInt());data或c_str函数能返回首地址
// 该函数在LogicSystem中被调用
void CSession::Send(const char *msg, short max_len, short msg_id)
{
    std::lock_guard<std::mutex> lock(m_SendLock);
    int sendQueueLen = m_SendQue.size();
    if (sendQueueLen > MAX_SENDQUE) {
        std::cout << "session: " << m_Uuid << "   send queue fulled, size now is " << MAX_SENDQUE
                  << std::endl;
        return;
    }

    //构造一个新的节点用于传输信息，并push进入队列
    m_SendQue.push(std::make_shared<SendNode>(msg, max_len, msg_id));

    // 如果发送队列之前不为空，说明已经有其他消息在等待发送，当前线程不需要处理发送
    if (sendQueueLen > 0) {
        return;
    }

    auto &msgnode = m_SendQue.front();

    //socket.async_send适合UDP
    boost::asio::async_write(m_Socket,
                             boost::asio::buffer(msgnode->m_Data, msgnode->m_TotalLen),
                             std::bind(&CSession::HandleWrite,
                                       this,
                                       std::placeholders::_1,
                                       SharedSelf()));
}

void CSession::SendHttp(std::string &msg)
{
    //不需要构造SendNode,所以不需要调用函数时指定Msg_Id,在构造Json数据的时候就写入Msg_Id

    std::string target = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: "
                         + std::to_string(msg.size())
                         + "\r\n"
                           "Connection: keep-alive\r\n"
                           "\r\n"
                         + msg;
    // boost::asio::async_write(m_Socket, buffer(target),
    //                          [this](boost::system::error_code ec, size_t) {
    //     if (ec) {
    //         std::cerr << "Error sending HTTP response: " << ec.message() << std::endl;
    //         return;
    //     }
    //     std::cout << "Sent HTTP response" << std::endl;
    // });
    boost::asio::async_write(m_Socket,
                             boost::asio::buffer(target),
                             std::bind(&CSession::HandleWrite,
                                       this,
                                       std::placeholders::_1,
                                       SharedSelf()));
}

void CSession::HandleWrite(const boost::system::error_code &error,
                           std::shared_ptr<CSession> shared_self)
{
    try {
        if (!error) {
            std::lock_guard<std::mutex> lock(m_SendLock);
            // std::cout << "send data " << m_SendQue.front()->m_Data + HEAD_TOTAL_LEN << std::endl;
            m_SendQue.pop();
            //继续处理下一个队列中的信息。没有就不管，解锁。
            if (!m_SendQue.empty()) {
                auto &msgnode = m_SendQue.front();
                //不需要在这里监听读事件，在读完MsgNode的时候就已经开始读事件监听了

                boost::asio::async_write(m_Socket,
                                         boost::asio::buffer(msgnode->m_Data, msgnode->m_TotalLen),
                                         std::bind(&CSession::HandleWrite,
                                                   this,
                                                   std::placeholders::_1,
                                                   shared_self));
            }
        } else {
            std::cout << "handle write failed, error is " << error.what() << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
        }
    } catch (std::exception &e) {
        std::cerr << "Exception code : " << e.what() << std::endl;
    }
}
