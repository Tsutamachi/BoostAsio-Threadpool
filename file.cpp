#include "file.h"

FileToReceve::FileToReceve(short fileid,
                           std::string sessionUuid,
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
{}

void FileToReceve::FlushToDisk()
{
    //tocheck
    while (true) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_CV.wait(lock, [this] {
            return m_ReceivedFlags[m_NextExpectedSeq % WINDOW_SIZE];
        }); //这是在查询有无符合要求的数据包？

        // 1. 写入数据
        int buffer_pos = m_NextExpectedSeq % WINDOW_SIZE;
        m_FileSaveStream.write(m_DataBuffer[buffer_pos].data(), m_DataBuffer[buffer_pos].size());

        // 2. 重置位标记
        m_ReceivedFlags.reset(buffer_pos);

        // 3. 移动窗口
        m_NextExpectedSeq++;

        // 文件传输完成检测
        if (m_NextExpectedSeq >= m_FilePacketsNum) {
            m_FileSaveStream.close();
            break;
        }
    }
}

void FileToReceve::CheckMissingPackets()
{
    //tocheck
    std::vector<int> missing_seqs;
    for (int i = 0; i < WINDOW_SIZE; ++i) {
        // 计算绝对序号
        int seq = m_NextExpectedSeq + i;

        // 跳过已超出总包数的序号
        if (seq >= m_FileTotalPackets)
            break;

        // 检查位标记
        if (!m_ReceivedFlags.test(i)) {
            missing_seqs.push_back(seq);
        }
    }

    // if (!missing_seqs.empty()) {
    //     RequestRetransmission(missing_seqs);
    // }
}

FileToSend::FileToSend(std::string filename,
                       unsigned int filesize,
                       int filetotalpackets,
                       std::string filehash)
    : m_FileName(filename)
    , m_FileSize(filesize)
    , m_FileTotalPackets(filetotalpackets)
    , m_FileHash(filehash)
{}

void FileToSend::SetFileId(short fileid)
{
    m_FileId = fileid;
}
