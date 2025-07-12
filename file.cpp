#include "file.h"
#include "defines.h"
#include "hashmd5.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <json/value.h>
#include "filemanagement.h"
#include "base64_code.h"
#include <boost/asio.hpp>
#include <memory>

//这是接收文件的函数
FileToReceve::FileToReceve(short fileid,
                           std::string &sessionUuid,
                           std::string filename,
                           uintmax_t filesize,
                           int filenum,
                           std::string filehash,
                           std::shared_ptr<CSession> session)
    : m_SessionUuid(sessionUuid)
    , m_FileId(fileid)
    , m_FileName(filename)
    , m_FileSize(filesize)
    , m_FileTotalPackets(filenum)
    , m_FileHash(filehash)
    , m_NextExpectedSeq(0)
    , m_AllReceivedFlags(filenum, false)
    , m_WindowStart(0)
    , m_CheckedReceiveSeq(0)
    , m_Verify((filenum / 10) + 1, false)
    , m_HashCodes((filenum / 10) + 1)
    , m_NextVerifying(0)
    , m_RewriteIndex(0)
    , m_Session(session)
    , b_AbleToReceiveLostBag(true)
    , b_FlushFinished(false)
    , b_IsFinalAccepted(false)

{
    try {
        m_FileSavePath = std::filesystem::path(DataPlace) / filename;
        std::cout << "FilePath in Server: " << m_FileSavePath << std::endl;
        //使用 std::ios::trunc 会截断已存在的文件，或者创建新文件;没有trunc的话不会创建新的文件。
        m_FileSaveStream.open(m_FileSavePath, std::ios::in | std::ios::out | std::ios::trunc);
        if (!m_FileSaveStream.is_open()) {
            std::error_code ec(errno, std::generic_category());
            std::cerr << "m_FileSaveStream to init file: " << ec.message() << std::endl;
            throw std::runtime_error("m_FileSaveStream init fails!");
        }

        // 启动 FlushToDisk 线程
        // std::thread([this] { FlushToDisk(); }).detach(); //这里不能用join--why?-->join会阻塞等待子线程的结束，detach不会
        m_FlushToDiskThread.start([this]{//不捕获this,找不到FlushToDisk
            std::cout<<"m_FlushToDiskThread start!"<<std::endl;
            FlushToDisk(); });
        m_FlushToDiskThread.detach();
        // std::thread([this] { VerifyHash(); }).detach();//已经被写到LogicSystem适当的位置
        // std::thread([this] { ReWriteCausedByHash(); }).detach();//已经被写到LogicSystem适当的位置
    } catch (std::system_error &e) {
        std::cout << e.what() << std::endl;
    }
}

FileToReceve::~FileToReceve()
{
    // {
    //     std::lock_guard<std::mutex> lock(m_Mutex);
    //     // m_StopFlush = true; // 新增成员变量 bool m_StopFlush = false;
    //     m_CV.notify_all();
    // }
    // // 等待线程结束（若使用join）
    // if (m_FlushThread.joinable()) m_FlushThread.join();
    // 关闭文件流
    if (m_FileSaveStream.is_open()) {
        m_FileSaveStream.close();
    }
    if (m_VerifyStream.is_open()) {
        m_VerifyStream.close();
    }

    // 清理动态分配的资源，例如 shared_ptr 会自动减少引用计数，并在需要时释放对象
    // 如果有其他动态分配的资源，也应该在这里释放

    // 销毁条件变量和互斥锁（如果它们是动态分配的）
    // 注意：在C++标准库中，std::mutex 和 std::condition_variable 不需要显式销毁
    // 它们会在作用域结束时自动销毁。只有当你动态分配它们时才需要这样做。

    // 清空所有容器
    m_DataBuffer.fill({});
    m_AllReceivedFlags.clear();
    m_MissingSeqs.clear();
    m_Verify.clear();
    m_HashCodes.clear();
    m_DamagedBlock.clear();
    m_HashDatas.clear();
}

//与FileManagement::AddPacket搭配使用条件变量m_CV
void FileToReceve::FlushToDisk()
{
    int i = 0;
    while (true) {
        try {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CV.wait(lock, [this] {
                return m_AllReceivedFlags[m_NextExpectedSeq];
            }); //查询有无符合要求的数据包

            if (!m_FileSaveStream.is_open()) {
                std::cout << "m_FileSaveStream open fails!" << std::endl;
                throw std::runtime_error("m_FileSaveStream open fails!");
            }

            // 写入数据
            int buffer_pos = m_NextExpectedSeq % WINDOW_SIZE;
            // std::cout << "FlushToDisk持久化文件数据量：" << m_DataBuffer[buffer_pos].size()<< std::endl << std::endl;
            m_FileSaveStream.write(m_DataBuffer[buffer_pos].data(), m_DataBuffer[buffer_pos].size());

            m_NextExpectedSeq++;

            std::cout<<"\nFlushed Seq: "<<i++<<std::endl<<"Flushed Size: "<<m_DataBuffer[buffer_pos].size()<<std::endl<<std::endl;

            // 文件传输完成检测
            if (m_NextExpectedSeq >= m_FileTotalPackets) {
                //这里做close的话，会与ServerHandleFinalBag中一起导致m_FileSaveStream的double free
                //->可能是因为FlushtoDisk是一个子线程，对m_FileSaveStream访问出了问题

                // if(m_FileSaveStream.is_open()){
                    // m_FileSaveStream.close();
                    // std::cout<<"FlushtuDisk fd close!"<<std::endl;
                // }
                std::cout<<"FlushToDisk Finish!"<<std::endl;
                b_FlushFinished = true;
                m_CVFlushed.notify_one();
                break;
            }
        } catch (std::system_error &e) {
            std::cout << e.what() << std::endl;
        }
    }

}

//在滑动窗口中检测缺包
bool FileToReceve::CheckMissingPackets()
{
    //应该从第一个没有确认接收的包的序号（已确认的最高连续序号。）开始，到WindowStart之间
    std::vector<uintmax_t> missing_seqs;
    for (uintmax_t i = m_WindowStart; i < m_LastReceivedSeq; ++i) {
        if (i >= m_FileTotalPackets)
            break;

        // 检查位标记
        if (!m_AllReceivedFlags[i]) {
            missing_seqs.push_back(i);
        }
    }

    if (!missing_seqs.empty()) {
        m_MissingSeqs = missing_seqs;
        return true;
    }
    return false;
}

bool FileToReceve::FinalCheck()
{
    try {
        // flag检测缺包
        bool flag = CheckMissingPackets();
        if (flag) {
            // 有缺包
            auto MissingSeqs = GetMissingSeqs();

            Json::Value Msg;
            std::cout << "Missing bag sequences :" << std::endl;
            for (auto seq : MissingSeqs) {
                std::cout << seq << "'\t";
                Msg["MissingBags"].append(seq); //Json数组
            }
            std::cout<<std::endl;

            Msg["Fileid"] = m_FileId;
            std::string target = Msg.toStyledString();

            m_Session->Send(target.data(), target.size(), TellLostBag);
            return false;
        } else {
            //确保检测完整性前，已经全部Flush
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CVFlushed.wait(lock, [this] {
                return b_FlushFinished;
            });

            //没有缺包，hashMD5检测完整性-complete
            if (m_FileSaveStream.is_open()){
                m_FileSaveStream.close();
                // std::cout<<"ServerHandleFinalBag fd close!"<<std::endl;
            }

            //FileSavePath后期可能需要改正
            bool complete = VerifyFileHash(m_FileSavePath, m_FileHash);
            if (complete) {
                //Hash没有问题
                Json::Value Msg;
                Msg["FileId"] = m_FileId;
                Msg["Missing"] = 0;
                std::string target = Msg.toStyledString();
                //这里可以rename修改 文件在Server存储的文件路径
                m_Session->Send(target.data(), target.size(), FileComplete);

                m_Session->m_FileIds[m_FileId] = true;
                FileManagement::GetInstance()->removeFile(m_Session->GetUuid(), m_FileId);
                std::cout << "File upload complete! FileName: " << m_FileName << std::endl;
                m_FileTimer.stop();
                std::cout << "耗时: " << m_FileTimer.getMilliseconds() << " ms\n";
                return true;
            } else {
                // hash验证失败--请求分块检验
                Json::Value Msg;
                Msg["FileId"] = m_FileId;
                std::string target = Msg.toStyledString();

                m_Session->Send(target.data(), target.size(), RequestVerify);
                return false;
            }
        }

    } catch (std::system_error &e) {
        std::cout << e.what() << std::endl;
    }
    return false;
}

// bool FileToReceve::CheckMissingPacketsInAll()
// {
//     for (int i = 0; i < m_FileTotalPackets; ++i) {
//         // 检查位标记
//         if (!m_AllReceivedFlags[i]) {
//             m_MissingSeqs.push_back(i);
//         }
//     }

//     if (m_MissingSeqs.size() != 0)
//         return true;
//     else
//         return false;
// }

const std::vector<uintmax_t> &FileToReceve::GetMissingSeqs()
{
    return m_MissingSeqs;
}

void FileToReceve::SlideWindow()
{
    // 将窗口滑动到第一个未接受
    while (m_AllReceivedFlags[m_WindowStart]) {
        m_WindowStart++;
    }
}

void FileToReceve::AddHashCode(std::string clientcode, uintmax_t seq)
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    // std::cout<<"FileToReceve::AddHashCode start"<<std::endl;

    //两个容器的容量都是 m_FileTotalPackets / 10（可能+1） 且连续
    if (seq >= m_Verify.size()) {
        // 扩展 m_Verify 到 seq+1，新增元素默认初始化为 false
        m_Verify.resize(seq + 1, false);
        // 扩展 m_HashCodes 到 seq+1，新增元素默认初始化为空字符串
        m_HashCodes.resize(seq + 1);
    }

    // 标记该序号已添加
    m_Verify[seq] = true;
    // 记录对应的哈希码
    m_HashCodes[seq] = clientcode;
    m_CVVerify.notify_one();
}

void FileToReceve::VerifyHash()
{
    // std::cout<<"VerifyHash Thread Start!"<<std::endl;
    std::vector<char> dataBuffer(Hash_Verify_Block);
    uintmax_t seq = 0;
    uintmax_t total = 0;
    uintmax_t seqtotal = m_FileTotalPackets / 10 +1;
    while (true) {
        try {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CVVerify.wait(lock, [this] { return m_Verify[m_NextVerifying]; });
            m_VerifyStream.seekg(m_NextVerifying * Hash_Verify_Block);

            //一次检测10个包
            m_VerifyStream.read(reinterpret_cast<char *>(dataBuffer.data()),
                                Hash_Verify_Block);
            size_t bytes_read = m_VerifyStream.gcount();
            // if (bytes_read == 0)
            //     continue;
            total += bytes_read;

            std::string data = CalculateBlockHash(dataBuffer);

            if (m_HashCodes[m_NextVerifying] != data) {
                //有问题
                std::cout<<"m_HashCodes["<<m_NextVerifying<<"] 检测有问题"<<std::endl;
                m_DamagedBlock.push_back(m_NextVerifying);
            }
            else{
                std::cout<<"m_HashCodes["<<m_NextVerifying<<"] 检测OK"<<std::endl;
            }

            m_NextVerifying++;
            seq++;
            dataBuffer.clear();


            /*if (m_VerifyStream.eof())*/
            if (seq >= seqtotal){
                std::cout<<"m_VerifyStream.eof()"<<std::endl;
                //检查剩余所有包
                // if (total < m_FileSize) {
                //     std::string data = CalculateBlockHash(dataBuffer);

                //     if (m_HashCodes[m_NextVerifying] == data) {
                //         //没问题
                //         m_HashCodes[m_NextVerifying] = "true";
                //     } else {
                //         //有问题
                //         m_DamagedBlock.push_back(m_NextVerifying);
                //     }

                //     m_NextVerifying++;
                //     seq++;
                //     dataBuffer.clear();
                //     // break;
                // }
                // unsigned int bags = m_FileTotalPackets / 10;
                // if (m_FileTotalPackets % 10)
                //     bags++;
                // 文件传输完成检测

                /*if (m_NextVerifying >= bags)*/
                {
                    if(m_VerifyStream.is_open())
                        m_VerifyStream.close();

                    std::cout << "Hash Verified!" << std::endl;

                    // std::unique_lock<std::mutex> lock(file->m_Mutex);
                    // file->m_VerifyFinish.wait(lock);
                    Json::Value Msg;
                    Msg["Fileid"] = m_FileId;
                    std::cout << "Missing bag sequences :" << std::endl;
                    for (auto seq : m_DamagedBlock) {
                        std::cout << seq << "'\t";
                        Msg["MissingBags"].append(seq); //Json数组
                    }
                    std::string target = Msg.toStyledString();
                    m_Session->Send(target.data(), target.size(), SendDamagedBlock);
                    break;
                }
            }

        } catch (std::system_error &e) {
            std::cout << e.what() << std::endl;
        }
    }
}

void FileToReceve::ReWriteCausedByHash()
{
    std::cout<<"ReWriteCausedByHash Thread Start!"<<std::endl;
    while (true) {
        try {
            if(m_FileSaveStream.is_open())
                m_FileSaveStream.close();
            m_FileSaveStream.open(m_FileSavePath,
                                  std::ios::in | std::ios::out
                                  | std::ios::binary); //同时启用读写权限，避免覆盖写时自动清空文件

            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CVRewrite.wait(lock, [this] {
                return (m_RewriteIndex < m_HashDatas.size())
                        && (!m_HashDatas[m_RewriteIndex].data.empty());
            }); //查询有无符合要求的数据包

            if (!m_FileSaveStream.is_open()) {
                std::cout << "ReWriteCausedByHash: m_FileSaveStream open fails!" << std::endl;
                throw std::runtime_error("ReWriteCausedByHash: m_FileSaveStream open fails!");
            }

            std::cout << "Received file sequence :" << m_HashDatas[m_RewriteIndex].seq << std::endl;

            size_t start = m_HashDatas[m_RewriteIndex].seq * FILE_DATA_LEN; //需要+1吗
            m_FileSaveStream.seekp(start);                                  // 定位到文件开头
            m_FileSaveStream.write(m_HashDatas[m_RewriteIndex].data.data(),
                                   m_HashDatas[m_RewriteIndex].data.size()); //覆盖写

            m_RewriteIndex++;

            //防止Rewrite处理太快，m_HashDatas还没添加完就被判定长度相同了
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (m_RewriteIndex >= m_HashDatas.size()) {
                std::cout << "ReWrite Finished!" << std::endl;
                m_FileSaveStream.close();
                //检测现在的文件是否完整
                if (VerifyFileHash(m_FileSavePath, m_FileHash)) {
                    Json::Value Msg;
                    Msg["FileId"] = m_FileId;
                    Msg["Missing"] = 0;
                    std::string target = Msg.toStyledString();
                    m_Session->Send(target.data(), target.size(), FileComplete);
                } else {
                    //重置资源
                    m_Verify.clear();
                    m_HashCodes.clear();
                    m_NextVerifying = 0;
                    m_DamagedBlock.clear();
                    m_HashDatas.clear();
                    m_RewriteIndex = 0;

                    Json::Value Msg;
                    Msg["FileId"] = m_FileId;
                    std::string target = Msg.toStyledString();

                    m_Session->Send(target.data(), target.size(), RequestVerify);
                }
                break;
            }
        } catch (std::system_error &e) {
            std::cout << e.what() << std::endl;
        }
    }
}


//之后的函数是Client端发送文件的函数
FileToSend::FileToSend(std::string filepath,
                       std::string filename,
                       uintmax_t filesize,
                       uintmax_t filetotalpackets,
                       std::string filehash,
                       std::ifstream file,
                       std::shared_ptr<CSession> session)
    : m_FilePath(filepath)
    , m_FileName(filename)
    , m_FileSize(filesize)
    , m_FileTotalPackets(filetotalpackets)
    , m_FileHash(filehash)
    , m_FileUploadStream(std::move(file))
    , m_FinishCheck(false)
    , m_Session(session)
{}

FileToSend::~FileToSend()
{
    if (m_FileUploadStream.is_open()) {
        m_FileUploadStream.close();
    }
}

void FileToSend::SetFileId(short fileid)
{
    m_FileId = fileid;
}

void FileToSend::StartFinishTimer(std::shared_ptr<CSession> session)
{
    m_FinishTimer->expires_after(std::chrono::seconds(20)); // 20 秒超时

    m_FinishTimer->async_wait([this, session](const boost::system::error_code &ec) {
        if (!ec && !m_FinishCheck) {
            // 异步重启定时器
            // boost::asio::post(m_FinishTimer->get_executor(), [this]() { StartFinishTimer(); });
            // return true;
            Json::Value Finish;
            Finish["FileId"] = m_FileId;
            Finish["Filefinish"] = 1;
            std::string target = Finish.toStyledString();
            session->Send(target.data(), target.size(), FileFinish);
            StartFinishTimer(session); // 重启定时器
        }
    });
}

void FileToSend::StopFinishTimer()
{
    if (m_FinishTimer) {
        m_FinishTimer->cancel();
    }
    m_FinishCheck = true; // 标记已收到确认
}

void FileToSend::SendFile()
{
    std::vector<char> dataBuffer(FILE_DATA_LEN);
    uintmax_t seq = 0;
    uintmax_t total = 0;

    while (m_FileUploadStream.read(reinterpret_cast<char *>(dataBuffer.data()),
                                               FILE_DATA_LEN)) {
        size_t bytes_read = m_FileUploadStream.gcount(); //读取的字符数
        if (bytes_read == 0)
            break;
        // std::cout << "本次读取了 ：" << bytes_read << " 个字节。" << std::endl;
        total += bytes_read;

        // 构造 JSON 数据
        Json::Value Msg;
        Msg["FileId"] = m_FileId;
        // Msg["Sequence"] = static_cast<Json::UInt>(seq);
        Msg["Sequence"] = seq;
        // Data为Base64 编码(利用openssl/BoostAsio库中函数自己实现编码).Json不能传送二进制文件，只能传文本
        Msg["Data"] = base64_encode(dataBuffer.data(), bytes_read);

        std::string target = Msg.toStyledString();

        if (target.size() > std::numeric_limits<short>::max()) { //max为32767
            throw std::runtime_error("Packet too large");
        }

        //这里主动插入一个缺包(第10个包)进行调试
        if(seq <= 19){
            seq++;
            dataBuffer.clear();
            // continue;
        }else{
            // 加入发送队列
            m_Session->Send(target.data(), target.size(), FileDataBag);
            std::cout << "Send sequence :" << seq << std::endl;
            // std::cout << "Send length :" << target.size() << std::endl;
            std::cout << "Send length :" << bytes_read << std::endl;

            seq++;
            dataBuffer.clear();
        }
    }

    if ( m_FileUploadStream.eof()) {
        //检查是否需要最后一个包
        if (total <  m_FileSize) {
            Json::Value Msg;
            Msg["FileId"] = m_FileId;
            Msg["Sequence"] = seq;

            size_t last_bytes_read =  m_FileUploadStream.gcount();
            if (last_bytes_read > 0) { // 确保读取了数据
                Msg["Data"] = base64_encode(dataBuffer.data(), last_bytes_read);

                std::string target = Msg.toStyledString();
                if (target.size() > std::numeric_limits<short>::max()) {
                    throw std::runtime_error("Packet too large");
                }

                std::cout << "Send sequence :" << seq << std::endl;
                std::cout << "Send length :" << last_bytes_read << std::endl;
                m_Session->Send(target.data(), target.size(), FileDataBag);

                seq++;
                dataBuffer.clear();
            }
            std::cout << "File eof!" << std::endl;
        }
    }
    //关闭文件
    if (m_FileUploadStream.is_open()) {
        m_FileUploadStream.close();
    }

    //发送完后需要再接收一个Server确认文件完整的包。在获得hash确认完整后再在Client的File队列中删除该File
    Json::Value Finish;
    Finish["FileId"] = m_FileId;
    Finish["Filefinish"] = 1;
    std::string target1 = Finish.toStyledString();
    std::cout << "Send finished." << std::endl;
    m_Session->Send(target1.data(), target1.size(), FileFinish);
    //如果时间内（20s）Server没有收到FileFinish,要重发这个包(boostasio中有定时器)
    m_FinishTimer = std::make_shared<boost::asio::steady_timer>(m_Session->GetIoContext());
    StartFinishTimer(m_Session);
}

