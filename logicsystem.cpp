#include "logicsystem.h"
#include "base64_code.h"
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

//这一层的数据处理，只需要考虑数据包部分，包头由MegNode进行自动封装
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
                // std::cout << std::endl
                //           << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

                if (meg_node->m_Session->m_Role == CSession::Role::Client) {
                    auto call_back_iter = m_ClientFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                    if (call_back_iter
                        == m_ClientFunCallBacks
                               .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                        m_MegQue.pop();
                        std::cout << "No compared CallBackFunction to handle this bag!"
                                  << std::endl;
                        continue;
                    }

                    //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                    call_back_iter->second(meg_node->m_Session,
                                           std::string(meg_node->m_RecevNode->m_Data,
                                                       meg_node->m_RecevNode
                                                           ->m_TotalLen)); //深度拷贝RecevNode中的Data
                    m_MegQue.pop();
                } else {
                    auto call_back_iter = m_ServerFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                    if (call_back_iter
                        == m_ServerFunCallBacks
                               .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                        m_MegQue.pop();
                        std::cout << "No compared CallBackFunction to handle this bag!"
                                  << std::endl;
                        continue;
                    }

                    //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                    call_back_iter->second(meg_node->m_Session,
                                           std::string(meg_node->m_RecevNode->m_Data,
                                                       meg_node->m_RecevNode
                                                           ->m_TotalLen)); //深度拷贝RecevNode中的Data
                    m_MegQue.pop();
                }
            }
            break;
        }

        // //正在传输
        // auto meg_node = m_MegQue.front();
        // std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

        // auto call_back_iter = m_FunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
        // if (call_back_iter
        //     == m_FunCallBacks
        //            .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
        //     m_MegQue.pop();
        //     continue;
        // }

        // //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
        // call_back_iter
        //     ->second(meg_node->m_Session,
        //              // meg_node->m_RecevNode->m_MsgId,
        //              std::string(meg_node->m_RecevNode->m_Data,
        //                          meg_node->m_RecevNode->m_TotalLen)); //深度拷贝RecevNode中的Data
        // m_MegQue.pop();
        {
            auto meg_node = m_MegQue.front();
            // std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

            if (meg_node->m_Session->m_Role == CSession::Role::Client) {
                auto call_back_iter = m_ClientFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                if (call_back_iter
                    == m_ClientFunCallBacks
                           .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                    m_MegQue.pop();
                    std::cout << "No compared CallBackFunction to handle this bag!" << std::endl;
                    continue;
                }

                //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                call_back_iter->second(meg_node->m_Session,
                                       std::string(meg_node->m_RecevNode->m_Data,
                                                   meg_node->m_RecevNode
                                                       ->m_TotalLen)); //深度拷贝RecevNode中的Data
                m_MegQue.pop();
            } else {
                auto call_back_iter = m_ServerFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                if (call_back_iter
                    == m_ServerFunCallBacks
                           .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                    m_MegQue.pop();
                    std::cout << "No compared CallBackFunction to handle this bag!" << std::endl;
                    continue;
                }

                //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                call_back_iter->second(meg_node->m_Session,
                                       std::string(meg_node->m_RecevNode->m_Data,
                                                   meg_node->m_RecevNode
                                                       ->m_TotalLen)); //深度拷贝RecevNode中的Data
                m_MegQue.pop();
            }
        }
    }
}

void LogicSystem::RegisterCallBacks()
{
    //后续可拓展消息id对应的回调函数

    m_ClientFunCallBacks[Test] = std::bind(&LogicSystem::ClientSendTest,
                                           this,
                                           std::placeholders::_1,
                                           std::placeholders::_2);

    m_ServerFunCallBacks[Echo] = std::bind(&LogicSystem::ServerSendTest,
                                           this,
                                           std::placeholders::_1,
                                           std::placeholders::_2);
    m_ClientFunCallBacks[Back] = std::bind(&LogicSystem::ClientReturn,
                                           this,
                                           std::placeholders::_1,
                                           std::placeholders::_2);

    m_ClientFunCallBacks[FileUploadRequest] = std::bind(&LogicSystem::RequestUpload,
                                                        this,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2);

    m_ClientFunCallBacks[RejectUpload] = std::bind(&LogicSystem::HandleRejectUpload,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2);

    m_ServerFunCallBacks[RequestFileId] = std::bind(&LogicSystem::HandleUploadRequest,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2);

    m_ClientFunCallBacks[StartUpload] = std::bind(&LogicSystem::HandleFileUpload,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2);

    m_ServerFunCallBacks[FileDataBag] = std::bind(&LogicSystem::HandleData,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2);

    m_ServerFunCallBacks[FileFinish] = std::bind(&LogicSystem::HandleFinalBag,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);

    //双端通用函数RequestDownload需不需要单独设立一个FunCallBacks?
    m_ClientFunCallBacks[FileDownloadRequest] = std::bind(&LogicSystem::RequestDownload,
                                                          this,
                                                          std::placeholders::_1,
                                                          std::placeholders::_2);
    m_ServerFunCallBacks[FileDownloadRequest] = std::bind(&LogicSystem::RequestDownload,
                                                          this,
                                                          std::placeholders::_1,
                                                          std::placeholders::_2);
}

//Server只提供下载请求（Server1->Server2），Client只提供上传请求(Client->Server)

//Client函数   Client端发出Test
//收: Test
//发：Echo
void LogicSystem::ClientSendTest(std::shared_ptr<CSession> session, const std::string &msg_data)
{ //echo模式，回显回到Client端
    session->Send(msg_data.data(), msg_data.size(), Echo);
}

//Server函数
void LogicSystem::ServerSendTest(std::shared_ptr<CSession> session, const std::string &msg_data)
{ //echo模式，回显回到Client端
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string

    std::cout /*<< "Session: " << session->m_Uuid << std::endl*/
        << "recevied msg id   is " << root["id"].asInt() << std::endl
        << "         msg data is " << root["data"].asString() << std::endl;

    std::string return_str = root.toStyledString();
    session->Send(return_str.data(), return_str.size(), Back);
}

//Client函数      Client端的回显测试
//收：Eco
void LogicSystem::ClientReturn(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string

    std::cout << "recevied msg id   is " << root["id"].asInt() << std::endl
              << "         msg data is " << root["data"].asString() << std::endl;
}

//Client函数      Client端提出上传请求
//收：FileUploadRequest
//发：RequestFileId
void LogicSystem::RequestUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //读取路径
    Json::Reader reader;
    Json::Value Msg;
    reader.parse(msg_data, Msg);
    std::string filepath = Msg["filepath"].asString();

    //获取文件的Hash值用于Server验证文件是否传输成功
    std::string filehash = CalculateFileHash(filepath);

    //先定位到文末，计算大小，再重置到文初
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Client open file failed: " + filepath);
    }

    //获取文件名
    std::filesystem::path pathObj(filepath);
    std::string filename = pathObj.filename().string();

    // 获取文件大小
    unsigned int file_size = file.tellg();
    file.seekg(0);

    //计算总包数
    unsigned int total_packets = (file_size + FILE_DATA_LEN - 1) / FILE_DATA_LEN;

    session->GetClientOwner()->m_TempFile = std::make_unique<FileToSend>(filepath,
                                                                         filename,
                                                                         file_size,
                                                                         total_packets,
                                                                         filehash,
                                                                         std::move(file));

    //构造要传给Server的数据
    Json::Value root;
    root["FileName"] = filename;
    root["FileSize"] = file_size;
    root["TotalPacketsNum"] = total_packets;
    root["FileHash"] = filehash;
    std::string target = root.toStyledString();

    // short msg_id = boost::asio::detail::socket_ops::host_to_network_short(FileUploadRequest);
    // short msg_len = boost::asio::detail::socket_ops::host_to_network_shor(target.size());

    // auto packet = std::make_shared<std::vector<char>>(len); //为什么这里使用共享指针？

    // size_t offset = 0;
    // memcpy(packet->data() + offset, &net_msg_id, HEAD_ID_LEN);
    // offset += HEAD_ID_LEN;
    // memcpy(packet->data() + offset, &net_msg_len, HEAD_DATA_LEN);
    // offset += HEAD_DATA_LEN;
    // memcpy(packet->data() + offset, target.data(), target.size());

    // boost::asio::async_write(Client.GetSocket(),
    //                          boost::asio::buffer(packet.data(), target.size() + HEAD_TOTAL_LEN),
    //                          std::bind(&CSession::HandleReadHead,
    //                                    this, //这里应该是Client端创建的CSession对象，并不是this
    //                                    std::placeholders::_1,
    //                                    std::placeholders::_2,
    //                                    SharedSelf()));
    session->Send(target.data(), target.size(), RequestFileId);
}

//Client、RecevServer函数   提出下载请求
//发：FileDownRequest
void LogicSystem::RequestDownload(std::shared_ptr<CSession> session, const std::string &msg_data) {}

//Server函数      处理上传请求
//收：RequestFileId
//发：StartUpload（true）        RejectUpload（flase）
void LogicSystem::HandleUploadRequest(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //读取获得的文件信息
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std::string filename = root["FileName"].asString();
    unsigned int filesize = root["FileSize"].asUInt();
    unsigned int filenum = root["TotalPacketsNum"].asUInt(); //总的包数
    std::string filehash = root["FileHash"].asString();
    std::cout << std::endl
              << "File Name:\t" << filename << std::endl
              << "File Size:\t" << filesize << std::endl
              << "Packet Num:\t" << filenum << std::endl;

    //由Session分配一个FileId
    short fileid = session->GetFileId(); //从0开始到4
    //创建File对象用于接收文件数据包和持久化文件
    if (!(fileid + 1)) {
        std::cout << "File upload num is full. Client can only upload 5 files in the same time!"
                  << std::endl;
        //回包拒绝
        Json::Value Reject;
        Reject["reject"] = 1;
        std::string target = Reject.toStyledString();

        session->Send(target.data(), target.length(), RejectUpload);
        return;

    } else {
        std::unique_ptr<FileToReceve> file = std::make_unique<FileToReceve>(fileid,
                                                                            session->GetUuid(),
                                                                            filename,
                                                                            filesize,
                                                                            filenum,
                                                                            filehash);
        FileManagement::GetInstance()->AddFile(session->GetUuid(), fileid, std::move(file));
    }

    // char send_data[TOTAL_LEN] = {0};
    Json::Value Msg;
    Msg["FileId"] = fileid;
    std::string request = Msg.toStyledString();

    // int msgid = boost::asio::detail::socket_ops::host_to_network_short(ReturnFileId);
    // memcpy(send_data, &msgid, HEAD_ID_LEN);

    // size_t request_length = request.length();
    // int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
    // memcpy(send_data + HEAD_ID_LEN, &request_host_length, HEAD_DATA_LEN);

    // memcpy(send_data + HEAD_TOTAL_LEN, request.data(), request_length);

    session->Send(request.data(), request.length(), StartUpload);
}

//Client函数      处理被拒绝的上传
//收：RejectUpload
void LogicSystem::HandleRejectUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //关闭文件❌->文件还没创建
    std::cout << "File upload num is full. Client can only upload 5 files in the same time!"
              << std::endl;
}

//Client函数   Client接受Server返回的上传响应--开始上传数据--最后投递到发送队列的部分可能有错误
//收：StartUpload
//发：FileDataBag  、FileFinish
void LogicSystem::HandleFileUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //接受数据FileId
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "JSON parse error" << std::endl;
        return;
    }

    short fileid = static_cast<short>(root["FileId"].asInt());
    if (fileid < 0) {
        std::cerr << "FileId invalid!" << std::endl;
        return;
    }

    session->GetClientOwner()->m_TempFile->SetFileId(fileid);
    session->GetClientOwner()->AddFileToSend(std::move(session->GetClientOwner()->m_TempFile),
                                             fileid);

    //这里能用一个指针代替吗
    std::unique_ptr<FileToSend> &filetosend = session->GetClientOwner()->FindFileToSend(fileid);

    std::vector<char> dataBuffer(FILE_DATA_LEN);
    unsigned int seq = 0;

    while (filetosend->m_FileUploadStream.read(reinterpret_cast<char *>(dataBuffer.data()),
                                               FILE_DATA_LEN)) {
        size_t bytes_read = filetosend->m_FileUploadStream.gcount();

        // 构造 JSON 数据
        Json::Value Msg;
        Msg["FileId"] = fileid;
        Msg["Sequence"] = static_cast<Json::UInt>(seq);
        // Base64 编码(利用openssl/BoostAsio库中函数自己实现编码).Json不能传送二进制文件，只能传文本
        Msg["Data"] = base64_encode(dataBuffer.data(), bytes_read);

        std::string target = Msg.toStyledString();
        if (target.size() > std::numeric_limits<short>::max()) { //max为32767
            throw std::runtime_error("Packet too large");
        }

        // size_t msg_len = HEAD_TOTAL_LEN + target.size();
        // auto packet = std::make_shared<std::vector<char>>(len);

        // 填充包头
        // short net_msg_id = boost::asio::detail::socket_ops::host_to_network_short(FileDataBag);
        // short net_msg_len = boost::asio::detail::socket_ops::host_to_network_short(target.size());

        // size_t offset = 0;
        // memcpy(packet->data() + offset, &net_msg_id, sizeof(net_msg_id));
        // offset += sizeof(net_msg_id);
        // memcpy(packet->data() + offset, &net_msg_len, sizeof(net_msg_len));
        // offset += sizeof(net_msg_len);
        // memcpy(packet->data() + offset, target.data(), target.size());

        // 加入发送队列
        session->Send(target.data(), target.size(), FileDataBag);
        seq++;
        dataBuffer.clear();
    }
    filetosend->m_FileUploadStream.close();
    //发送完后需要再接收一个Server确认文件完整的包。在获得确认完整后再在Client的File队列中删除该File
    Json::Value Finish;
    Finish["FileId"] = fileid;
    Finish["Filefinish"] = 1;
    std::string target1 = Finish.toStyledString();
    session->Send(target1.data(), target1.size(), FileFinish);
    //如果时间内Server没有收到FileFinish,要重发这个包
}

//Server函数   Server接受数据
//收：FileDataBag
void LogicSystem::HandleData(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //接受数据FileId
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "JSON parse error" << std::endl;
        return;
    }

    short fileid = static_cast<short>(root["FileId"].asInt());
    unsigned int seq = root["Sequence"].asUInt();
    std::string data = root["Data"].asString();
    std::vector<char> filedata = base64_decode(data); //二进制文件

    FileManagement::GetInstance()->AddPacket(session->GetUuid(), fileid, seq, filedata);
}

//Server函数   Server收到Client端发送完毕的消息。
//收：FileFinish
//发：TellLostBag  or  FileComplete
void LogicSystem::HandleFinalBag(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //需要检验缺包
    //接受数据
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "JSON parse error" << std::endl;
        return;
    }
    short fileid = static_cast<short>(root["FileId"].asInt());
    // bool finish = root["Filefinish"].asBool();//没用的数据

    bool flag
        = FileManagement::GetInstance()->findFile(session->GetUuid(), fileid)->CheckMissingPackets();
    if (flag) {
        //这里检测缺包
        /*std::vector<long> *MissingSeqs
            = FileManagement::GetInstance()->findFile(session->GetUuid(), fileid)->GetMissingSeqs();*/
        std::vector<unsigned int> *MissingSeqs = FileManagement::GetInstance()
                                                     ->findFile(session->GetUuid(), fileid)
                                                     ->GetMissingSeqs(); // 统一类型

        std::string bagnums;
        for (size_t i = 0; i < MissingSeqs->size(); ++i) {
            bagnums.append(std::to_string((*MissingSeqs)[i])); // 正确解引用指针并访问元素
            if (i < MissingSeqs->size() - 1) {
                bagnums.append(",");
            }
        }

        Json::Value Msg;
        Msg["Fileid"] = fileid;
        Msg["MissingBags"] = bagnums;
        std::string target = Msg.toStyledString();

        session->Send(target.data(), target.size(), TellLostBag);
    } else {
        //没有缺包，hashMD5检测完整性
        bool complete = VerifyFileHash(
            //FileSavePath后期需要改正
            FileManagement::GetInstance()->findFile(session->GetUuid(), fileid)->m_FileSavePath,
            FileManagement::GetInstance()->findFile(session->GetUuid(), fileid)->m_FileHash);
        if (complete) {
            Json::Value Msg;
            Msg["Missing"] = 0;
            std::string target = Msg.toStyledString();

            session->Send(target.data(), target.size(), FileComplete);
        } else {
            //这里做不完整的处理--重发文件？
        }
    }
}

//Server->Client    Client需要重发缺包

//Server->Client    Client处理文件传输完成后的资源回收File
