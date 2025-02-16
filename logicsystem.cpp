#include "logicsystem.h"
#include "base64_code.h"
#include "csession.h"
#include "defines.h"
#include "file.h"
#include "filemanagement.h"
#include "hashmd5.h"
#include <filesystem>
#include <json/json.h>
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
                    if (call_back_iter == m_ServerFunCallBacks.end()) {
                        m_MegQue.pop();
                        std::cout << "No compared CallBackFunction to handle this bag!"
                                  << std::endl;
                        continue;
                    }

                    call_back_iter->second(meg_node->m_Session,
                                           std::string(meg_node->m_RecevNode->m_Data,
                                                       meg_node->m_RecevNode->m_TotalLen));
                    m_MegQue.pop();
                }
            }
            break;
        }

        //正在传输

        auto meg_node = m_MegQue.front();

        if (meg_node->m_Session->m_Role == CSession::Role::Client) {
            auto call_back_iter = m_ClientFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
            if (call_back_iter == m_ClientFunCallBacks.end()) {
                m_MegQue.pop();
                std::cout << "No compared CallBackFunction to handle this bag!" << std::endl;
                continue;
            }

            call_back_iter->second(meg_node->m_Session,
                                   std::string(meg_node->m_RecevNode->m_Data,
                                               meg_node->m_RecevNode->m_TotalLen));
            m_MegQue.pop();
        } else {
            auto call_back_iter = m_ServerFunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
            if (call_back_iter == m_ServerFunCallBacks.end()) {
                m_MegQue.pop();
                std::cout << "No compared CallBackFunction to handle this bag!" << std::endl;
                continue;
            }

            call_back_iter->second(meg_node->m_Session,
                                   std::string(meg_node->m_RecevNode->m_Data,
                                               meg_node->m_RecevNode->m_TotalLen));
            m_MegQue.pop();
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

    m_ServerFunCallBacks[FileFinish] = std::bind(&LogicSystem::ServerHandleFinalBag,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);

    m_ClientFunCallBacks[ServerRecevFinal] = std::bind(&LogicSystem::ClientHandleFinalBag,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2);

    m_ClientFunCallBacks[RequestVerify] = std::bind(&LogicSystem::SendVerifyCode,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2);

    m_ServerFunCallBacks[VerifyCode] = std::bind(&LogicSystem::ReceiveVerifyCode,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);

    m_ClientFunCallBacks[SendDamagedBlock] = std::bind(&LogicSystem::SendDamagedHashBlock,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2);

    m_ServerFunCallBacks[ReTranDamagedHash] = std::bind(&LogicSystem::ServerHandleDamagedHashBag,
                                                        this,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2);

    m_ClientFunCallBacks[TellLostBag] = std::bind(&LogicSystem::HandleReTransmit,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2);

    m_ClientFunCallBacks[FileComplete] = std::bind(&LogicSystem::FinishUpload,
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
{
    std::cout << "Received client send test1." << std::endl;
    session->Send(msg_data.data(), msg_data.size(), Echo);
}

//Server函数   Server将Test请求包回显给Client
//收：Echo
//发：Back
void LogicSystem::ServerSendTest(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string

    std::cout /*<< "Session: " << session->m_Uuid << std::endl*/
        << "received test1 from client" << std::endl
        << "test message data is     :" << root["data"].asString() << std::endl;

    std::string return_str = root.toStyledString();
    session->Send(return_str.data(), return_str.size(), Back);
}

//Client函数      Client端接收回显测试
//收：Eco
void LogicSystem::ClientReturn(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    std::cout << "Server have recevied test1." << std::endl
              << "message data is           :" << root["data"].asString() << std::endl;
}

//Client函数      Client端提出上传请求
//收：FileUploadRequest
//发：RequestFileId
void LogicSystem::RequestUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    try {
        //读取路径
        Json::Reader reader;
        Json::Value Msg;
        reader.parse(msg_data, Msg);
        if (!reader.parse(msg_data, Msg)) {
            std::cerr << "RequestUpload JSON parse error" << std::endl;
            return;
        }

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
        // unsigned int total_packets = (file_size + FILE_DATA_LEN - 1) / FILE_DATA_LEN;
        unsigned int total_packets = file_size / FILE_DATA_LEN;
        if ((file_size % FILE_DATA_LEN))
            total_packets++;

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

        std::cout << "Client request upload file:" << filename << std::endl;
        session->Send(target.data(), target.size(), RequestFileId);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
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
    if (!reader.parse(msg_data, root)) {
        std::cerr << "HandleUploadRequest JSON parse error" << std::endl;
        return;
    }

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
        std::unique_ptr<FileToReceve> file
            = std::make_unique<FileToReceve>(fileid,
                                             session->GetUuid(), //Server给传输数据的Client分配的Uuid
                                             filename,
                                             filesize,
                                             filenum,
                                             filehash,
                                             session);
        FileManagement::GetInstance()->AddFile(session->GetUuid(), fileid, std::move(file));
    }

    Json::Value Msg;
    Msg["FileId"] = fileid;
    std::string request = Msg.toStyledString();
    std::cout << "Received the upload request from client.File name:" << filename << std::endl;
    std::cout << "FileId :" << fileid << std::endl;
    session->Send(request.data(), request.length(), StartUpload);
    std::cout << "Start Receive File :" << filename << std::endl;
}

//Client函数      处理被拒绝的上传
//收：RejectUpload
void LogicSystem::HandleRejectUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //不需要关闭文件->文件还没创建
    std::cout << "Upload Request failed. Client can only upload " << MAX_UPLOAD_NUM
              << " files in the same time!" << std::endl;
}

//Client函数   Client开始上传数据
//收：StartUpload
//发：FileDataBag  、FileFinish
void LogicSystem::HandleFileUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //接受数据FileId
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "HandleFileUpload JSON parse error" << std::endl;
        return;
    }

    short fileid = static_cast<short>(root["FileId"].asInt());
    if (fileid < 0) {
        std::cerr << "FileId invalid!" << std::endl;
        return;
    }
    std::cout << "Received FileId is :" << fileid << std::endl;

    std::shared_ptr<FileToSend> tempfile = session->GetClientOwner()->m_TempFile;
    tempfile->SetFileId(fileid);
    session->GetClientOwner()->AddFileToSend(tempfile, fileid);

    std::shared_ptr<FileToSend> filetosend = session->GetClientOwner()->FindFileToSend(fileid);
    if (!filetosend) {
        std::cout << "LogicSystem::HandleFileUpload FindFileToSend fails" << std::endl;
        return;
    }

    std::cout << "Start Upload File :" << filetosend->m_FileName << std::endl << std::endl;
    if (!filetosend->m_FileUploadStream.is_open()) {
        filetosend->m_FileUploadStream.open(filetosend->m_FilePath);
    }

    std::vector<char> dataBuffer(FILE_DATA_LEN);
    unsigned int seq = 0;
    unsigned int total = 0;

    while (filetosend->m_FileUploadStream.read(reinterpret_cast<char *>(dataBuffer.data()),
                                               FILE_DATA_LEN)) {
        size_t bytes_read = filetosend->m_FileUploadStream.gcount(); //读取的字符数
        if (bytes_read == 0)
            break;
        // std::cout << "本次读取了 ：" << bytes_read << " 个字节。" << std::endl;
        total += bytes_read;

        // 构造 JSON 数据
        Json::Value Msg;
        Msg["FileId"] = fileid;
        // Msg["Sequence"] = static_cast<Json::UInt>(seq);
        Msg["Sequence"] = seq;
        // Base64 编码(利用openssl/BoostAsio库中函数自己实现编码).Json不能传送二进制文件，只能传文本
        Msg["Data"] = base64_encode(dataBuffer.data(), bytes_read);

        std::string target = Msg.toStyledString();
        if (target.size() > std::numeric_limits<short>::max()) { //max为32767
            throw std::runtime_error("Packet too large");
        }

        std::cout << "Send sequence :" << seq << std::endl;
        std::cout << "Send length :" << target.size() << std::endl;
        // 加入发送队列
        session->Send(target.data(), target.size(), FileDataBag);

        seq++;
        dataBuffer.clear();
    }

    if (filetosend->m_FileUploadStream.eof()) {
        //检查是否需要最后一个包
        if (total < filetosend->m_FileSize) {
            Json::Value Msg;
            Msg["FileId"] = fileid;
            Msg["Sequence"] = seq;

            size_t last_bytes_read = filetosend->m_FileUploadStream.gcount();
            if (last_bytes_read > 0) { // 确保读取了数据
                Msg["Data"] = base64_encode(dataBuffer.data(), last_bytes_read);

                std::string target = Msg.toStyledString();
                if (target.size() > std::numeric_limits<short>::max()) {
                    throw std::runtime_error("Packet too large");
                }

                std::cout << "Send sequence :" << seq << std::endl;
                session->Send(target.data(), target.size(), FileDataBag);

                seq++;
                dataBuffer.clear();
            }
            std::cout << "File eof!" << std::endl;
        }
    }

    //发送完后需要再接收一个Server确认文件完整的包。在获得hash确认完整后再在Client的File队列中删除该File
    filetosend->m_FileUploadStream.close();
    Json::Value Finish;
    Finish["FileId"] = fileid;
    Finish["Filefinish"] = 1;
    std::string target1 = Finish.toStyledString();
    std::cout << "Send finished." << std::endl;
    session->Send(target1.data(), target1.size(), FileFinish);
    //如果时间内Server没有收到FileFinish,要重发这个包(boostasio中有定时器)
    filetosend->m_FinishTimer = std::make_shared<boost::asio::steady_timer>(session->m_IoContext);
    filetosend->StartFinishTimer(session);
}

//Server函数   Server接受数据
//收：FileDataBag
//发：TellLostBag（缺包）
void LogicSystem::HandleData(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    //接受数据FileId
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "HandleData JSON parse error" << reader.getFormattedErrorMessages()
                  << std::endl;
        return;
    }

    short fileid = static_cast<short>(root["FileId"].asInt());
    unsigned int seq = root["Sequence"].asUInt();
    std::string data = root["Data"].asString();
    std::vector<char> filedata = base64_decode(data); //二进制文件
    // std::cout << "本次接受数据长度 :" << filedata.size() << std::endl;

    FileManagement::GetInstance()->AddPacket(session->GetUuid(), fileid, seq, filedata);
    std::cout << "Received file sequence :" << seq << std::endl;

    // 检测缺包
    std::shared_ptr<FileToReceve> file = FileManagement::GetInstance()->findFile(session->GetUuid(),
                                                                                 fileid);
    if (file->CheckMissingPackets()) {
        // 构造 TellLostBag 消息
        Json::Value Msg;
        Msg["Fileid"] = fileid;
        std::cout << "Missing bag sequences :" << std::endl;
        for (auto seq : file->m_MissingSeqs) {
            std::cout << seq << "'\t";
            Msg["MissingBags"].append(seq); //Json数组
        }
        std::cout << std::endl;
        std::string target = Msg.toStyledString();
        session->Send(target.data(), target.size(), TellLostBag);
    }
}

//Server函数   Server收到Client端发送完毕的消息。
//收：FileFinish
//发：TellLostBag  or  FileComplete
void LogicSystem::ServerHandleFinalBag(std::shared_ptr<CSession> session,
                                       const std::string &msg_data)
{
    //接受数据
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "ServerHandleFinalBag JSON parse error" << std::endl;
        return;
    }
    short fileid = static_cast<short>(root["FileId"].asInt());

    Json::Value recev;
    recev["FileId"] = fileid;
    std::string target1 = recev.toStyledString();
    session->Send(target1.data(), target1.size(), ServerRecevFinal);

    // flag检测缺包
    auto file = FileManagement::GetInstance()->findFile(session->GetUuid(), fileid);
    bool flag = file->CheckMissingPackets();
    if (flag) {
        // 有缺包
        auto MissingSeqs = file->GetMissingSeqs();

        Json::Value Msg;
        std::cout << "Missing bag sequences :" << std::endl;
        for (auto seq : file->m_MissingSeqs) {
            std::cout << seq << "'\t";
            Msg["MissingBags"].append(seq); //Json数组
        }

        Msg["Fileid"] = fileid;
        std::string target = Msg.toStyledString();

        session->Send(target.data(), target.size(), TellLostBag);
    } else {
        //没有缺包，hashMD5检测完整性-complete
        std::shared_ptr<FileToSend> filetosend = session->GetClientOwner()->FindFileToSend(fileid);
        if (!filetosend) {
            std::cout << "LogicSystem::SendVerifyCode FindFileToSend fails" << std::endl;
            return;
        }
        file->m_FileSaveStream.close();

        //FileSavePath后期可能需要改正
        bool complete = VerifyFileHash(file->m_FileSavePath, file->m_FileHash);
        if (complete) {
            //Hash没有问题
            Json::Value Msg;
            Msg["FileId"] = fileid;
            Msg["Missing"] = 0;
            std::string target = Msg.toStyledString();
            session->Send(target.data(), target.size(), FileComplete);
        } else {
            // hash验证失败--请求分块检验
            Json::Value Msg;
            Msg["FileId"] = fileid;
            std::string target = Msg.toStyledString();

            session->Send(target.data(), target.size(), RequestVerify);
        }
    }
}

//Server->Client    Client发送HASH验证码
//收：RequestVerify
//发：VerifyCode
void LogicSystem::SendVerifyCode(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "SendVerifyCode JSON parse error" << std::endl;
        return;
    }
    short fileid = static_cast<short>(root["FileId"].asInt());

    std::shared_ptr<FileToSend> filetosend = session->GetClientOwner()->FindFileToSend(fileid);
    if (!filetosend) {
        std::cout << "LogicSystem::SendVerifyCode FindFileToSend fails" << std::endl;
        return;
    }

    filetosend->m_FileUploadStream.open(filetosend->m_FilePath);
    if (!filetosend->m_FileUploadStream.is_open()) {
        std::cerr << "SendVerifyCode cannot open the file." << std::endl;
        return;
    }

    std::vector<char> dataBuffer(Hash_Verify_Block);
    unsigned int seq = 0;
    unsigned int total = 0;
    //一次检测10个包
    while (filetosend->m_FileUploadStream.read(reinterpret_cast<char *>(dataBuffer.data()),
                                               Hash_Verify_Block)) {
        size_t bytes_read = filetosend->m_FileUploadStream.gcount();
        if (bytes_read == 0)
            break;
        total += bytes_read;

        // 构造 JSON 数据
        Json::Value Msg;
        Msg["FileId"] = fileid;
        Msg["Sequence"] = seq;
        Msg["Data"] = CalculateBlockHash(dataBuffer);

        std::string target = Msg.toStyledString();
        if (target.size() > std::numeric_limits<short>::max()) { //max为32767
            throw std::runtime_error("Packet too large");
        }

        std::cout << "Send Hashsequence :" << seq << std::endl;
        // 加入发送队列
        session->Send(target.data(), target.size(), VerifyCode);

        seq++;
        dataBuffer.clear();
    }

    if (filetosend->m_FileUploadStream.eof()) {
        //检查是否需要最后一个包
        if (total < filetosend->m_FileSize) {
            Json::Value Msg;
            Msg["FileId"] = fileid;
            Msg["Sequence"] = seq;

            size_t last_bytes_read = filetosend->m_FileUploadStream.gcount();
            if (last_bytes_read > 0) { // 确保读取了数据
                Msg["Data"] = CalculateBlockHash(dataBuffer);
                std::string target = Msg.toStyledString();
                if (target.size() > std::numeric_limits<short>::max()) {
                    throw std::runtime_error("Packet too large");
                }

                std::cout << "Send sequence :" << seq << std::endl;
                session->Send(target.data(), target.size(), VerifyCode);

                dataBuffer.clear();
            }
            std::cout << "File eof!" << std::endl;
        }
    }
    filetosend->m_FileUploadStream.close();
}

//Client->Server    Server接受HASH验证码
//收：VerifyCode
//发：SendDamagedBlock
void LogicSystem::ReceiveVerifyCode(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "ReceiveVerifyCode JSON parse error" << std::endl;
        return;
    }

    short fileid = root["FileId"].asInt();
    unsigned int seq = root["Sequence"].asUInt();
    std::string hashdata = root["Data"].asString();
    // 添加数据到File中
    FileManagement::GetInstance()->findFile(session->GetUuid(), fileid)->AddHashCode(hashdata, seq);

    //所有发送接收后，会在对应的file中，发送SendDamagedBlock包
}

//Server->Client    接收Server发出的问题seq
//收：SendDamagedBlock
//发：ReTranDamagedHash
void LogicSystem::SendDamagedHashBlock(std::shared_ptr<CSession> session,
                                       const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "SendDamagedHashBlock JSON parse error" << std::endl;
        return;
    }
    short fileid = root["Fileid"].asInt();

    auto file = session->GetClientOwner()->FindFileToSend(fileid);
    if (!file) {
        std::cout << "LogicSystem::SendDamagedHashBlock FindFileToSend fails" << std::endl;
        return;
    }

    file->m_FileUploadStream.open(file->m_FilePath, std::ios::binary);
    if (!file->m_FileUploadStream.is_open()) {
        file->m_FileUploadStream.open(file->m_FilePath);
    }

    auto missing_seqs = root["MissingBags"];
    size_t total = 0;
    std::vector<char> buffer(FILE_DATA_LEN);
    for (auto seq_value : missing_seqs) {
        unsigned int seq = seq_value.asUInt();
        size_t start = seq * Hash_Verify_Block;
        file->m_FileUploadStream.seekg(start);

        unsigned int seqnow = seq * 10;
        for (int i = 0; i < 10; ++i) { //发10个包
            file->m_FileUploadStream.read(buffer.data(), FILE_DATA_LEN);
            size_t bytes_read = file->m_FileUploadStream.gcount();

            // 发送数据
            Json::Value Msg;
            Msg["FileId"] = fileid;
            Msg["Sequence"] = seqnow;
            Msg["Data"] = base64_encode(buffer.data(), bytes_read);

            std::string target = Msg.toStyledString();
            std::cout << "ReTransmit sequence :" << seqnow << std::endl;
            session->Send(target.data(), target.size(), ReTranDamagedHash);
            total += bytes_read;
            seqnow++;
        }
        if (file->m_FileUploadStream.eof()) {
            //检查是否需要最后一个包
            if (total < FILE_DATA_LEN) {
                Json::Value Msg;
                Msg["FileId"] = fileid;
                Msg["Sequence"] = seqnow;

                size_t last_bytes_read = file->m_FileUploadStream.gcount();
                if (last_bytes_read > 0) { // 确保读取了数据
                    Msg["Data"] = base64_encode(buffer.data(), last_bytes_read);
                    std::string target = Msg.toStyledString();
                    if (target.size() > std::numeric_limits<short>::max()) {
                        throw std::runtime_error("Packet too large");
                    }

                    std::cout << "ReTransmit sequence :" << seqnow << std::endl;
                    session->Send(target.data(), target.size(), ReTranDamagedHash);
                }
                std::cout << "File eof!" << std::endl;
            }
        }
    }
}

//Client->Server    Server接收 被重传的hash验证有问题的包 到文件（覆盖写）
//收：ReTranDamagedHash
void LogicSystem::ServerHandleDamagedHashBag(std::shared_ptr<CSession> session,
                                             const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "SendDamagedHashBlock JSON parse error" << std::endl;
        return;
    }
    short fileid = root["FileId"].asInt();
    unsigned int seq = root["Sequence"].asUInt();
    std::string data = root["Data"].asString();
    std::vector<char> filedata = base64_decode(data); //二进制文件

    FileManagement::GetInstance()->AddHashRetransmitDataPacket(session->GetUuid(),
                                                               fileid,
                                                               seq,
                                                               filedata);
}

//Server->Client    Client确认Server收到了FinalBag
//收：ServerRecevFinal
void LogicSystem::ClientHandleFinalBag(std::shared_ptr<CSession> session,
                                       const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "ClientHandleFinalBag JSON parse error" << std::endl;
        return;
    }
    short fileid = static_cast<short>(root["FileId"].asInt());

    auto file = session->GetClientOwner()->FindFileToSend(fileid);
    file->StopFinishTimer();
}

//Server->Client    Client重发缺包
//收：TellLostBag
void LogicSystem::HandleReTransmit(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "HandleReTransmit JSON parse error" << std::endl;
        return;
    }

    short fileid = root["Fileid"].asInt();
    auto missing_seqs = root["MissingBags"];

    auto file = session->GetClientOwner()->FindFileToSend(fileid);
    if (!file) {
        std::cout << "LogicSystem::HandleFileUpload FindFileToSend fails" << std::endl;
        return;
    }

    file->m_FileUploadStream.open(file->m_FilePath, std::ios::binary);

    std::vector<char> buffer(FILE_DATA_LEN);
    size_t bytes_read = 0;
    //重传
    for (auto seq_value : missing_seqs) {
        // 计算文件偏移量
        unsigned int seq = seq_value.asUInt();
        size_t start = seq * FILE_DATA_LEN;
        file->m_FileUploadStream.seekg(start);

        // 读取数据（处理最后一个包可能不足长度）
        file->m_FileUploadStream.read(buffer.data(), FILE_DATA_LEN);
        bytes_read = file->m_FileUploadStream.gcount();

        // 发送数据
        Json::Value Msg;
        Msg["FileId"] = fileid;
        Msg["Sequence"] = seq;
        Msg["Data"] = base64_encode(buffer.data(), bytes_read);

        std::string target = Msg.toStyledString();
        session->Send(target.data(), target.size(), FileDataBag);
        buffer.clear();

        if (file->m_FileUploadStream.eof()) {
            size_t size = file->m_FileUploadStream.gcount();
            //检查是否需要最后一个包
            if (size < FILE_DATA_LEN) {
                Json::Value Msg;
                Msg["FileId"] = fileid;
                Msg["Sequence"] = seq;

                if (size > 0) { // 确保读取了数据
                    Msg["Data"] = base64_encode(buffer.data(), size);
                    std::string target = Msg.toStyledString();
                    if (target.size() > std::numeric_limits<short>::max()) {
                        throw std::runtime_error("Packet too large");
                    }

                    std::cout << "ReTransmit sequence :" << seq << std::endl;
                    session->Send(target.data(), target.size(), ReTranDamagedHash);
                }
                std::cout << "File eof!" << std::endl;
            }
        }
    }

    file->m_FileUploadStream.close();
}

//Server->Client    Client处理文件传输完成后的资源回收File
//收：FileComplete
void LogicSystem::FinishUpload(std::shared_ptr<CSession> session, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        std::cerr << "FinishUpload JSON parse error" << std::endl;
        return;
    }
    short fileid = root["FileId"].asInt();
    session->GetClientOwner()->RemoveFile(fileid);
}
