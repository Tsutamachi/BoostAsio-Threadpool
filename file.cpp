#include "file.h"
#include "defines.h"

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
    , m_CheckAll(filenum)
{
    m_FileSavePath = DataPlace + filename;
}

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
        if (m_NextExpectedSeq >= m_FileTotalPackets) {
            m_FileSaveStream.close();
            break;
        }
    }
}

//在滑动窗口中检测缺包
bool FileToReceve::CheckMissingPackets()
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

bool FileToReceve::CheckMissingPacketsInAll()
{
    for (int i = 0; i < m_FileTotalPackets; ++i) {
        // 检查位标记
        if (!m_ReceivedFlags.test(i)) {
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

//之后的函数是Client端发送文件的函数
FileToSend::FileToSend(std::string filepath,
                       std::string filename,
                       unsigned int filesize,
                       int filetotalpackets,
                       std::string filehash,
                       std::ifstream file)
    : m_FilePath(filepath)
    , m_FileName(filename)
    , m_FileSize(filesize)
    , m_FileTotalPackets(filetotalpackets)
    , m_FileHash(filehash)
    , m_FileUploadStream(std::move(file))
{}

void FileToSend::SetFileId(short fileid)
{
    m_FileId = fileid;
}
