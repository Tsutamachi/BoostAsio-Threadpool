#pragma once
#include <array>
#include <bitset>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

//仿照LogicSystem的信息处理逻辑？在滑动窗口中找有无符合要求的数据包

//用于接收文件的File
class FileToReceve
{
    friend class FileManagement;

public:
    FileToReceve(short fileid,
                 std::string sessionUuid,
                 std::string filename,
                 unsigned int filesize,
                 int filenum,
                 std::string filehash);

    // 写入文件（按序处理）
    void FlushToDisk();

    void CheckMissingPackets();

    // 校验哈希
    bool ValidateHash(const std::string& client_hash);

private:
    std::string m_SessionUuid; //所属Session的Uuid
    short m_FileId;            //文件在该Session中的Id
    std::string m_FileName;    //文件名（UTF-8的256字节限定）
    unsigned int m_FileSize;   //文件总长度2**32
    unsigned int m_FileTotalPackets; //文件总包数

    static constexpr int WINDOW_SIZE = 1024;                 //滑动窗口的大小
    std::array<std::vector<char>, WINDOW_SIZE> m_DataBuffer; //滑动窗口
    std::bitset<WINDOW_SIZE> m_ReceivedFlags; //用来记录哪些窗口已经被载入 set1 resert0
    unsigned int m_NextExpectedSeq;           // 下一个期待的包序号

    // std::ofstream m_FileSaveStream; // 接收文件
    // std::ifstream m_FileUploadStream; //发送文件
    std::mutex m_Mutex;             // 缓冲区操作锁
    std::condition_variable m_CV;   // 条件变量
    std::string m_FileHash;         //文件验证哈希值
};

//用于发送的File
class FileToSend
{
    friend class FileManagement;

public:
    FileToSend(std::string filename,
               unsigned int filesize,
               int filetotalpackets,
               std::string filehash,
               std::ifstream file);
    void SetFileId(short fileid);

private:
    std::stirng m_FilePath;  //文件在Client中的路径，用于打开文件
    short m_FileId;          //文件在该Session中的Id
    std::string m_FileName;  //文件名（UTF-8的256字节限定）
    unsigned int m_FileSize; //文件总长度
    unsigned int m_FileTotalPackets; //文件总包数

    std::ifstream m_FileUploadStream; //发送文件
    std::mutex m_Mutex;               // 缓冲区操作锁
    std::condition_variable m_CV;     // 条件变量
    std::string m_FileHash;           //文件验证哈希值
};
