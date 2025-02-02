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

CSession::CSession(boost::asio::io_context &io_context, CServer *server)
    : m_Socket(io_context)
    , m_IoContext(io_context) //下文并未提及
    , m_Server(server)        //只起到关闭的作用
    , _close(false)
{
    boost::uuids::uuid am_Uuid = boost::uuids::random_generator()();
    m_Uuid = boost::uuids::to_string(am_Uuid);
    m_RecevHeadNode = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    for (short i = 0; i < MAX_UPLOAD_NUM; ++i) {
        m_FileIds[i] = true;
    }
}

CSession::CSession(boost::asio::ip::tcp::socket &socket, Client *client)
    : m_Socket(socket)
    , m_Client(client)
    , _close(false)
{}

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
{ //只有单例的LogicSystem会访问这个资源，不涉及多线程，所以不需要加锁
    for (short i = 0; i < MAX_UPLOAD_NUM; ++i) {
        if (m_FileIds[i] == true) {
            m_FileIds[i] = false;
            return i;
        }
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

void CSession::SocketClose()
{
    m_Socket.close();
}

void CSession::HandleReadHead(const boost::system::error_code &error,
                              size_t bytes_transferred,
                              std::shared_ptr<CSession> shared_self)
{
    try {
        if (error == boost::asio::error::eof) {
            // 连接被对端关闭。
            std::cerr << "Peer closed the connection." << std::endl;
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            //todo：这里要处理LogicSystem中在FileManagement里管理的Session下File资源？
            //没有return
        } else if (error) {
            //其他错误
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            std::cout << "ReadHead fails:" << error.what() << std::endl;
        }

        // std::cout << std::endl << "消息头部：" << std::endl;
        if (bytes_transferred < HEAD_TOTAL_LEN) {
            std::cout << "Read head length error" << std::endl;
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            return;
        }

        //如果有特殊要求：遇到某个字符就停止copy,可以用memccpy（Linux函数）.但是memcpy性能更好
        memcpy(m_RecevHeadNode->m_Data, m_Data, HEAD_TOTAL_LEN);
        m_RecevHeadNode->m_CurLen += bytes_transferred;

        short msg_id = 0;
        memcpy(&msg_id, m_RecevHeadNode->m_Data, HEAD_ID_LEN);
        msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
        if (msg_id > MAX_LENGTH) {
            std::cout << "invalid data id is :" << msg_id << std::endl;
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            return;
        }
        // std::cout << "MessageId:" << msg_id << std::endl;

        short msg_len = 0;
        memcpy(&msg_len, m_RecevHeadNode->m_Data + HEAD_ID_LEN, HEAD_DATA_LEN);
        msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
        if (msg_len > TOTAL_LEN) {
            std::cout << "invalid data length is :" << msg_len << std::endl;
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            return;
        }
        // std::cout << "MessageLength:" << msg_len << std::endl;

        // m_RecevHeadNode->Clear();
        m_RecevMsgNode = std::make_shared<RecevNode>(msg_len, msg_id);

        std::memset(m_Data, 0, HEAD_TOTAL_LEN);
        m_Socket.async_receive(boost::asio::buffer(m_Data, msg_len),
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
            SocketClose();
            m_Server->ClearSession(m_Uuid);
            //没有return
        }
        // std::cout << std::endl << "消息体:" << std::endl;
        memcpy(m_RecevMsgNode->m_Data, m_Data, m_RecevMsgNode->m_TotalLen);
        m_RecevMsgNode->m_CurLen += bytes_transferred;

        LogicSystem::GetInstance()->PostMesgToQue(
            make_shared<LogicNode>(shared_from_this(), m_RecevMsgNode));
        std::memset(m_Data, 0, HEAD_TOTAL_LEN);
        // m_RecevMsgNode->Clear();

        m_Socket.async_receive(boost::asio::buffer(m_Data, HEAD_TOTAL_LEN),
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
            SocketClose();
            m_Server->ClearSession(m_Uuid);
        }
    } catch (std::exception &e) {
        std::cerr << "Exception code : " << e.what() << std::endl;
    }
}
