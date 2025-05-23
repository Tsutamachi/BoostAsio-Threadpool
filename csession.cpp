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
// #include "filetransfer.h"

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
    , m_http_buffer(2048)

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
    //清空缓存区
    m_http_buffer.consume(m_http_buffer.size());
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

            //将请求类型导入到beast缓冲区
            boost::asio::buffer_copy( m_http_buffer.prepare(HEAD_TOTAL_LEN),
                                      boost::asio::buffer(m_Data, HEAD_TOTAL_LEN));
            m_http_buffer.commit(HEAD_TOTAL_LEN);

            std::cout << "Detected HTTP protocol" << std::endl;

            m_http_parser.emplace();
            m_http_parser->eager(true);
            //接收剩余的http数据
            StartHttpProcessing(shared_self);

        } else {
            std::cout << "Detected custom protocol" << std::endl;
            this->HandleMyProtocol(error, bytes_transferred, SharedSelf());
        }

    } catch (std::exception &e) {
        std::cerr << "HandleReadHead fails.Exception code is:" << e.what() << std::endl;
    }
}

void CSession::StartHttpProcessing(std::shared_ptr<CSession> self) {
    // m_http_parser->body_limit(1024 * 1024); // 按需设置

    // 持续异步读取直到完整请求头到达
    boost::beast::http::async_read(
                m_Socket,
                m_http_buffer,
                *m_http_parser,
                [this, self](boost::beast::error_code ec, size_t bytes) {
        if (ec == boost::beast::http::error::need_more) {
            // 需要更多数据：重新调度异步读取
            StartHttpProcessing(self);
            return;
        }
        // std::cout<<"Processing has been finished. turn to HttpProtocol "<<std::endl;
        HandleHttpProtocol(ec, bytes, self);
    }
    );
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
            // if (ec == boost::beast::http::error::end_of_stream) {
            //     return Start(); // 保持连接
            // }
            if (auto server = GetServerOwner()) {
                server->ClearSession(m_Uuid); // Server 端清理
                SocketClose();
            } else if (auto client = GetClientOwner()) {
                client->HandleSessionClose(); // Client 端清理（需在 Client 类中实现）
            }
            return;
        }

        // 从解析器获取请求对象
        m_http_request = m_http_parser->get();

        // 打印完整的HTTP请求
        // std::cout << "Received HTTP request:\n"
        //           << m_http_parser->get() << std::endl;

        m_http_response.version(m_http_request.version());//设置Http版本
        m_http_response.keep_alive(false);//短连接

        if(m_http_request.method() == boost::beast::http::verb::get){
            // std::cout<<"CSession.HandleHttpProtocol Triggerred!"<<std::endl;
            PreParseGetParam();
            // std::cout<<"HttpProtocol: _get_url = "<<_get_url<<std::endl;
            bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
            if (!success) {
                m_http_response.result(boost::beast::http::status::not_found);
                m_http_response.set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(m_http_response.body()) << "url not found\r\n";
                WriteResponse(SharedSelf());
                return;
            }

            m_http_response.result(boost::beast::http::status::ok);
            m_http_response.set(boost::beast::http::field::server, "GateServer");
            WriteResponse(SharedSelf());
            return;
        }

        else if(m_http_request.method() == boost::beast::http::verb::post){
            PreParseGetParam();
            // std::cout<<"HttpProtocol: _get_url = "<<_get_url<<std::endl;
            bool success = LogicSystem::GetInstance()->HandlePost(m_http_request.target(), shared_from_this());
            if (!success) {
                m_http_response.result(boost::beast::http::status::not_found);
                m_http_response.set(boost::beast::http::field::content_type, "text/plain");
                boost::beast::ostream(m_http_response.body()) << "url not found\r\n";
                WriteResponse(SharedSelf());
                return;
            }

            m_http_response.result(boost::beast::http::status::ok);
            m_http_response.set(boost::beast::http::field::server, "GateServer");
            WriteResponse(SharedSelf());
            return;
        }
        else{
            // 不支持的请求方法
            m_http_response.result(boost::beast::http::status::bad_request);
            m_http_response.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(m_http_response.body()) << "Unsupported HTTP method\r\n";
            WriteResponse(SharedSelf());
            return;
        }


        {
            //     if(!memcmp(m_RecevHeadNode->m_Data, "GET ", HEAD_TOTAL_LEN))
            //     {
            //         //处理GET请求:找出GET请求的文件名，传输（做成一个通用函数）
            //         std::string request_path = get_Http_FileName(m_HttpBuffer);
            //         std::cout<<"FilePath: "<<request_path<<std::endl;

            //         //不允许查看父目录
            //         if (request_path.find("..") != std::string::npos) {
            //             // send_http_error(403, "Forbidden");
            //             return;
            //         }

            //         std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
            //         boost::asio::async_write(m_Socket,
            //                                  boost::asio::buffer(response));
            //         // SendHttp(response);

            //         // FileTransfer::SendFile(this->m_Socket,request_path);
            //     }
            //     else if(!memcmp(m_RecevHeadNode->m_Data, "POST", HEAD_TOTAL_LEN))
            //     {
            //         //POST的处理：找出length，接收数据后送给logicSystem做处理
            //         size_t header_end = m_HttpBuffer.find("\r\n\r\n"); //返回的索引是指向第一个\r的索引
            //         std::string headers = m_HttpBuffer.substr(0, header_end); //创造一个header副本
            //         std::istringstream stream(headers);
            //         std::string line;
            //         unsigned long msg_len = 0;

            //         //获取Msg部分的长度msg_len
            //         while (std::getline(stream, line)) {
            //             std::transform(line.begin(), line.end(), line.begin(), ::tolower); // 处理大小写问题
            //             if (line.find("content-length:") != std::string::npos) {           //找到了
            //                 // +2跳过“:”和“空格”
            //                 msg_len = std::stoul(line.substr(line.find(":") + 2)); //string to unsigned long;
            //                 break;
            //             }
            //         }

            //         if (msg_len) {
            //             m_RecevMsgNode = std::make_shared<RecevNode>(msg_len,
            //                                                          0); //msg_id在读取信息里重新设置

            //             std::memset(m_Data, 0, HEAD_TOTAL_LEN);
            //             boost::asio::async_read(m_Socket,
            //                                     boost::asio::buffer(m_Data, msg_len), //m_Data[2048]  2kb
            //                                     std::bind(&CSession::HandleHttpReadMeg,
            //                                               this,
            //                                               std::placeholders::_1,
            //                                               std::placeholders::_2,
            //                                               SharedSelf()));
            //         }
            //     }

        }

    } catch (std::exception &e) {
        std::cerr << "HandleHttpReadMsg fails.\n  Exception code is:" << e.what() << std::endl;
    }
}

void CSession::WriteResponse(std::shared_ptr<CSession> shared_self) {
    // auto self = shared_from_this();

    m_http_response.content_length(m_http_response.body().size());

    boost::beast::http::async_write(
                m_Socket,
                m_http_response,
                [shared_self](boost::beast::error_code ec, std::size_t)
    {
        shared_self->m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        shared_self->deadline_.cancel();
    });
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

//char 转为16进制
unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

//16进制转为char
unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符
            strTemp += "+";
        else
        {
            //其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

void CSession::PreParseGetParam() {
    // https://search.bilibili.com/all?vt=54413020&keyword=llfc&from_source=webtop_search&spm_id_from=333.1007&search_source=5
    // 提取 URI
    auto uri = m_http_request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        _get_url = uri;
        return;
    }
    // search.bilibili.com/all
    _get_url = uri.substr(0, query_pos);//包左不包右

    //  vt=54413020&keyword=llfc&from_source=webtop_search&spm_id_from=333.1007&search_source=5
    // key= value  &    key=value&       key=value        &        key=value   &          key=value
    std::string query_string = uri.substr(query_pos + 1);

    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        // vt=54413020
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码
            value = UrlDecode(pair.substr(eq_pos + 1));
            _get_params[key] = value;
        }
        //（vt=54413020&）keyword=llfc&from_source=webtop_search&spm_id_from=333.1007&search_source=5
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}
