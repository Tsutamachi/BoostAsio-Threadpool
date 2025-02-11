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
    return shared_from_this();
}

short CSession::GetFileId()
{
    std::unique_lock<std::mutex> lock(m_IdLock);
    if (m_NextFileId != MAX_UPLOAD_NUM && m_FileIds[m_NextFileId] == true) //只会分配第一个0 1~4
    {
        short ret = m_NextFileId;
        m_NextFileId = (m_NextFileId + 1) % MAX_UPLOAD_NUM;
        return ret;
    } else if (m_NextFileId == MAX_UPLOAD_NUM && m_FileIds[0] == true) {
        m_NextFileId = 0; //分配除第一个0以外的所有0
        return m_NextFileId;
    }
    return -1;
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

    {
        //Edition2--协程
        // auto shared_this = shared_from_this();
        // //开启接受协程
        // boost::asio::co_spawn(
        //     m_IoContext,
        //     [shared_this, this]() -> awaitable<void> {
        //         try {
        //             while (!_close) {
        //                 m_RecevHeadNode->Clear();
        //                 std::size_t n = co_await boost::asio::async_read(
        //                     m_Socket,
        //                     boost::asio::buffer(m_RecevHeadNode->m_Data, HEAD_TOTAL_LEN),
        //                     use_awaitable);
        //                 if (n == 0) {
        //                     std::cout << "receive peer closed" << std::endl;
        //                     SocketClose();
        //                     m_Server->ClearSession(m_Uuid);
        //                     co_return;
        //                 }

        //                 //MSG_ID
        //                 short msg_id = 0;
        //                 memcpy(&msg_id, m_RecevHeadNode->m_Data, HEAD_ID_LEN);
        //                 msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
        //                 if (msg_id > MAX_LENGTH) {
        //                     std::cout << "invalid msg_id!" << std::endl;
        //                     SocketClose();
        //                     m_Server->ClearSession(m_Uuid);
        //                     co_return;
        //                 }
        //                 //MSG_LEN
        //                 short msg_len = 0;
        //                 memcpy(&msg_len, m_RecevHeadNode->m_Data + HEAD_ID_LEN, HEAD_DATA_LEN);
        //                 msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
        //                 if (msg_len > MAX_LENGTH) {
        //                     std::cout << "invalid msg_len!" << std::endl;
        //                     SocketClose();
        //                     m_Server->ClearSession(m_Uuid);
        //                     co_return;
        //                 }

        //                 //MSG
        //                 m_RecevMsgNode = std::make_shared<RecevNode>(msg_len, msg_id);
        //                 n = co_await boost::asio::async_read(m_Socket,
        //                                                      boost::asio::buffer(m_RecevMsgNode->m_Data,
        //                                                                          m_RecevMsgNode
        //                                                                              ->m_TotalLen),
        //                                                      use_awaitable);
        //                 if (n == 0) {
        //                     std::cout << "receive peer closed" << std::endl;
        //                     SocketClose();
        //                     m_Server->ClearSession(m_Uuid);
        //                     co_return;
        //                 }
        //                 m_RecevMsgNode->m_Data[m_RecevMsgNode->m_TotalLen] = '\0';
        //                 std::cout << "Recev Data is: " << m_RecevMsgNode->m_Data << std::endl;
        //                 LogicSystem::GetInstance()->PostMesgToQue(
        //                     std::make_shared<LogicNode>(shared_from_this(), m_RecevMsgNode));
        //             }

        //         } catch (std::system_error &e) {
        //             std::cout << "exception is " << e.what() << std::endl;
        //             SocketClose();
        //             m_Server->ClearSession(m_Uuid);
        //         }
        //     },
        //     detached);
    }
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
        // 连接被对端关闭。
        /*if (error == boost::asio::error::eof) {
            std::cerr << "Peer closed the connection." << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            //todo：这里要处理LogicSystem中在FileManagement里管理的Session下File资源？
            //没有return
        }
        //其他错误
        else */
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

        short msg_id = 0;
        memcpy(&msg_id, m_RecevHeadNode->m_Data, HEAD_ID_LEN);
        msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
        // std::cout << "CSession--msg_id_to_host :" << msg_id << std::endl;
        if (msg_id > 2020 || msg_id < 1001) {
            std::cout << "invalid data id is :" << msg_id << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            return;
        }
        // std::cout << "MessageId:" << msg_id << std::endl;

        short msg_len = 0;
        memcpy(&msg_len, m_RecevHeadNode->m_Data + HEAD_ID_LEN, HEAD_DATA_LEN);
        msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
        // std::cout << "CSession--msg_len_to_host :" << msg_len << std::endl << std::endl;
        if (msg_len > 2000) {
            std::cout << "invalid data length is :" << msg_len << std::endl;
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            return;
        }
        // std::cout << "MessageLength:" << msg_len << std::endl;

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
    } catch (std::exception &e) {
        std::cerr << "HandleReadHead fails.Exception code is:" << e.what() << std::endl;
    }
}

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
