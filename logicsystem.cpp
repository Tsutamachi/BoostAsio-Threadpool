#include "logicsystem.h"
#include "csession.h"
#include "defines.h"
#include "file.h"
#include "filemanagement.h"
#include "hashmd5.h"
#include <filesystem>
#include <json-c/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <string>

LogicSystem::LogicSystem()
    : m_stop(false)
{
    RegisterCallBacks();
    m_WorkerThread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    m_stop = true;
    m_Consume.notify_one(); //最后唤醒一次线程处理剩余没发送的信息
    m_WorkerThread.join();  //等待线程结束再回收资源
}

void LogicSystem::PostMesgToQue(std::shared_ptr<LogicNode> node)
{
    std::unique_lock<std::mutex> unique_lk(m_Mutex);
    m_MegQue.push(node);

    //队列如果是从0->1,则发送通知信号
    if (m_MegQue.size() == 1) {
        unique_lk.unlock();
        m_Consume.notify_one();
    }
}

void LogicSystem::DealMsg()
{
    while (true) {
        std::unique_lock<std::mutex> unique_lk(m_Mutex);

        //同时满足 1、队列为空 2、没有停止传输(正在传输) 则用条件变量unique_lk阻塞等待，并释放锁
        while (m_MegQue.empty() && !m_stop) {
            m_Consume.wait(unique_lk);
        }

        //停止了传输 && 队列中仍有信息
        if (m_stop) {
            while (!m_MegQue.empty()) {
                auto meg_node = m_MegQue.front();
                std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

                auto call_back_iter = m_FunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                if (call_back_iter
                    == m_FunCallBacks
                           .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                    m_MegQue.pop();
                    continue;
                }

                //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                call_back_iter->second(meg_node->m_Session,
                                       std::string(meg_node->m_RecevNode->m_Data,
                                                   meg_node->m_RecevNode
                                                       ->m_CurLen)); //深度拷贝RecevNode中的Data
                m_MegQue.pop();
            }
            break;
        }

        //正在传输
        auto meg_node = m_MegQue.front();
        std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

        auto call_back_iter = m_FunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
        if (call_back_iter
            == m_FunCallBacks
                   .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
            m_MegQue.pop();
            continue;
        }

        //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
        call_back_iter
            ->second(meg_node->m_Session,
                     // meg_node->m_RecevNode->m_MsgId,
                     std::string(meg_node->m_RecevNode->m_Data,
                                 meg_node->m_RecevNode->m_CurLen)); //深度拷贝RecevNode中的Data
        m_MegQue.pop();
    }
}

void LogicSystem::RegisterCallBacks()
{
    //后续可拓展消息id对应的回调函数
    m_FunCallBacks[Test] = std::bind(&LogicSystem::HandleTest,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2);
    m_FunCallBacks[FileFirstBag] = std::bind(&LogicSystem::HandleReadFirstFileBag,
                                             this,
                                             std::placeholders::_1,
                                             std::placeholders::_2);
}

//Server只提供下载请求（Server1->Server2），Client只提供上传请求(Client->Server)

//Server接受test
void LogicSystem::HandleTest(std::shared_ptr<CSession> session, const std::string &msg_data)
{ //echo模式，回显回到Client端
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string

    std::cout /*<< "Session: " << session->m_Uuid << std::endl*/
        << "recevied msg id   is " << root["id"].asInt() << std::endl
        << "         msg data is " << root["data"].asString() << std::endl;

    std::string return_str = root.toStyledString();
    session->Send(return_str.data(), return_str.size(), (short) root["id"].asInt());
}

//Client端提出上传请求--隶属于CSession层--与Client对象相关的部分可能有错误
void LogicSystem::RequestUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    std::string filepath;
    std::cout << "Please enter the path of file you want to upload: ";
    std::getline(std::cin, filepath);

    //先定位到文末，计算大小，再重置到文初
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Client open file failed: " + filepath);
    }

    //获取文件名 而非路径
    std::filesystem::path pathObj(filepath);
    std::string filename = pathObj.filename().string();

    // 获取文件大小
    size_t file_size = file.tellg();
    file.seekg(0);

    //计算总包数
    size_t total_packets = (file_size + FILE_DATA_LEN - 1) / FILE_DATA_LEN;

    //获取文件的Hash值用于Server验证文件是否传输成功
    std::string filehash = HashMD5::CalculateFileHash(filepath);

    Client.m_TempFile = std::make_unique<FileToSend>(filename, file_size, total_packets, filehash);

    short msg_id = boost::asio::detail::socket_ops::host_to_network_short(FileFirstBag);

    Json::Value root;
    root["FileName"] = filename;
    root["FileSize"] = file_size;
    root["TotalPacketsNum"] = total_packets;
    root["FileHash"] = filehash;
    std::string target = root.toStyledString();

    short msg_len = boost::asio::detail::socket_ops::host_to_network_shor(target.size());

    auto packet = std::make_shared<std::vector<char>>(len);

    size_t offset = 0;
    memcpy(packet->data() + offset, &net_msg_id, HEAD_ID_LEN);
    offset += HEAD_ID_LEN;
    memcpy(packet->data() + offset, &net_msg_len, HEAD_DATA_LEN);
    offset += HEAD_DATA_LEN;
    memcpy(packet->data() + offset, target.data(), target.size());

    boost::asio::async_write(Client.GetSocket(),
                             boost::asio::buffer(packet.data(), target.size() + HEAD_TOTAL_LEN),
                             std::bind(&CSession::HandleReadHead,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       SharedSelf()));
}

//Server处理Client发出的上传请求
void LogicSystem::HandleReadFirstFileBag(std::shared_ptr<CSession> session,
                                         const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string}
    std::string filename = root["FileName"].asString();
    std::string filehash = root["FileHash"].asString();
    unsigned int filesize = root["FileSize"].asInt();
    int filenum = root["PacketsNum"].asInt(); //总的包数
    std::cout << "File Name:\t" << filename << std::endl
              << "File Size:\t" << filesize << std::endl
              << "Packet Num:\t" << filenum << std::endl;

    //由Session分配一个FileId
    short fileid = session->GetFileId(); //从0开始到4
    if (!(fileid + 1)) {
        std::cout << "File upload num is full. Client can only upload 5 files in the same time!"
                  << std::endl;
    } else { //FileId有效才创建对象
        std::unique_ptr<FileToReceve> file = std::make_unique<FileToReceve>(fileid,
                                                                            session->GetUuid(),
                                                                            filename,
                                                                            filesize,
                                                                            filenum,
                                                                            filehash);
        FileManagement::GetInstance()->AddFile(session->GetUuid(), fileid, std::move(file));
    }

    char send_data[TOTAL_LEN] = {0};
    Json::Value Msg;
    Msg["FileId"] = fileid;
    std::string request = Msg.toStyledString();

    int msgid = ReturnFileId;
    msgid = boost::asio::detail::socket_ops::host_to_network_short(msgid);
    memcpy(send_data, &msgid, HEAD_ID_LEN);

    size_t request_length = request.length();
    int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
    memcpy(send_data + HEAD_ID_LEN, &request_host_length, HEAD_DATA_LEN);

    int seq = 0;
    seq = boost::asio::detail::socket_ops::host_to_network_short(seq);
    memcpy(send_data + HEAD_ID_LEN + HEAD_DATA_LEN, &seq, HEAD_SEQUENCE_LEN);

    memcpy(send_data + HEAD_TOTAL_LEN, request.c_str(), request_length);
    boost::asio::write(session->GetSocket(),
                       boost::asio::buffer(send_data, request_length + HEAD_TOTAL_LEN));
}

//Client接受Server返回的上传响应--开始上传数据--最后投递到发送队列的部分可能有错误
void LogicSystem::HandleReturnFirstId(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //接受数据FileId
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "JSON parse error" << std::endl;
        return;
    }

    short fileid = root["FileId"].asInt(); // 使用 asInt() 替代 asShort()
    if (fileid < 0) {
        std::cerr << "FileId invalid!" << std::endl;
        return;
    }

    std::vector<char> dataBuffer(FILE_DATA_LEN);
    size_t seq = 1;

    while (file.read(dataBuffer.data(), FILE_DATA_LEN)) {
        size_t bytes_read = file.gcount();

        // 构造 JSON 数据
        Json::Value Msg;
        Msg["FileId"] = fileid;
        Msg["Sequence"] = static_cast<Json::UInt>(seq);
        Msg["Data"] = base64_encode(dataBuffer.data(), bytes_read); // Base64 编码

        std::string target = Msg.toStyledString();
        if (target.size() > std::numeric_limits<short>::max()) {
            throw std::runtime_error("Packet too large");
        }

        size_t msg_len = HEAD_TOTAL_LEN + target.size();
        auto packet = std::make_shared<std::vector<char>>(len);

        // 填充包头
        short net_msg_id = boost::asio::detail::socket_ops::host_to_network_short(FileDataBag);
        short net_msg_len = boost::asio::detail::socket_ops::host_to_network_short(target.size());

        size_t offset = 0;
        memcpy(packet->data() + offset, &net_msg_id, sizeof(net_msg_id));
        offset += sizeof(net_msg_id);
        memcpy(packet->data() + offset, &net_msg_len, sizeof(net_msg_len));
        offset += sizeof(net_msg_len);
        memcpy(packet->data() + offset, target.data(), target.size());

        // 加入发送队列
        session->Send(packet->data(), packet->size(), FileDataBag);
        seq++;
    }

    file.close();
}
