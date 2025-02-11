#include "file.h"
#include "defines.h"
#include <filesystem>
#include <iostream>
#include <json/value.h>

//这是接收文件的函数
FileToReceve::FileToReceve(short fileid,
                           std::string &sessionUuid,
                           std::string filename,
                           unsigned int filesize,
                           int filenum,
                           std::string filehash)
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
{
    try {
        m_FileSavePath = std::filesystem::path(DataPlace) / filename;
        m_FileSaveStream.open(m_FileSavePath);
        if (!m_FileSaveStream.is_open()) {
            std::cout << "m_FileSaveStream init fails!" << std::endl;
            throw std::runtime_error("m_FileSaveStream init fails!");
        }
        // 启动 FlushToDisk 线程
        std::thread([this] { FlushToDisk(); }).detach();
    } catch (std::system_error &e) {
        std::cout << e.what() << std::endl;
    }
}

FileToReceve::~FileToReceve()
{
    if (m_FileSaveStream.is_open()) {
        m_FileSaveStream.close();
    }
}

//与FileManagement::AddPacket搭配使用条件变量m_CV
void FileToReceve::FlushToDisk()
{
    //tocheck
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

            // 1. 写入数据
            int buffer_pos = m_NextExpectedSeq % WINDOW_SIZE;
            std::cout << "FlushToDisk持久化文件数据量：" << m_DataBuffer[buffer_pos].size()
                      << std::endl
                      << std::endl;
            m_FileSaveStream.write(m_DataBuffer[buffer_pos].data(), m_DataBuffer[buffer_pos].size());

            // 2. 重置位标记
            // m_AllReceivedFlags[buffer_pos] = false;

            // 3. 移动窗口
            m_NextExpectedSeq++;

            // 文件传输完成检测
            if (m_NextExpectedSeq >= m_FileTotalPackets) {
                m_FileSaveStream.close();
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
    std::vector<unsigned int> missing_seqs;
    for (unsigned int i = m_WindowStart; i < m_LastReceivedSeq; ++i) {
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

bool FileToReceve::CheckMissingPacketsInAll()
{
    for (int i = 0; i < m_FileTotalPackets; ++i) {
        // 检查位标记
        if (!m_AllReceivedFlags[i]) {
            m_MissingSeqs.push_back(i);
        }
    }

    if (m_MissingSeqs.size() != 0)
        return true;
    else
        return false;
}

std::vector<unsigned int> *FileToReceve::GetMissingSeqs()
{
    return &m_MissingSeqs;
}

void FileToReceve::SlideWindow()
{
    // 将窗口滑动到第一个未接受
    while (m_AllReceivedFlags[m_WindowStart]) {
        m_WindowStart++;
    }
}

//之后的函数是Client端发送文件的函数
FileToSend::FileToSend(std::string filepath,
                       std::string filename,
                       unsigned int filesize,
                       unsigned int filetotalpackets,
                       std::string filehash,
                       std::ifstream file)
    : m_FilePath(filepath)
    , m_FileName(filename)
    , m_FileSize(filesize)
    , m_FileTotalPackets(filetotalpackets)
    , m_FileHash(filehash)
    , m_FileUploadStream(std::move(file))
    , m_FinishCheck(false)
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
