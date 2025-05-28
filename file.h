#pragma once
#include "csession.h"
#include <array>
#include <bitset>
#include <boost/asio.hpp>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "delayedthread.h"

struct HashRetransmitData
{
    unsigned int seq;
    const std::vector<char>& data;
};

//仿照LogicSystem的信息处理逻辑,在滑动窗口中找有无符合要求的数据包
//用于接收文件的File
class CSession;
class FileToReceve
{
    friend class FileManagement;
    friend class LogicSystem;

public:
    FileToReceve(short fileid,
                 std::string& sessionUuid,
                 std::string filename,
                 unsigned int filesize,
                 int filenum,
                 std::string filehash,
                 std::shared_ptr<CSession> session);
    ~FileToReceve();

    // 写入文件（按序处理）
    void FlushToDisk();

    bool CheckMissingPackets(); //滑动窗口多次检查缺包

    bool CheckMissingPacketsInAll(); //一次总检查缺包

    const std::vector<unsigned int>& GetMissingSeqs(); //一次总检查缺包

    void SlideWindow();

    void AddHashCode(std::string clientcode, unsigned int seq);

    void VerifyHash();

    void ReWriteCausedByHash();

private:
    DelayedThread m_FlushToDiskThread;
    DelayedThread m_VerifyHashThread;
    DelayedThread m_ReWriteCausedByHashThread;

    //文件信息
    std::string m_SessionUuid;       //所属Session的Uuid
    short m_FileId;                  //文件在该Session中的Id
    std::string m_FileName;          //文件名（UTF-8的256字节限定）
    unsigned int m_FileSize;         //文件总长度2**32
    unsigned int m_FileTotalPackets; //文件总包数
    std::string m_FileHash;          //文件验证哈希值
    std::string m_FileSavePath;      //Data文件夹的位置+文件名
    std::shared_ptr<CSession> m_Session; //在Server中所属的Session

    //发送文件过程中，缺包检测
    int m_WindowStart;                                       //滑动窗口的起点
    static constexpr int WINDOW_SIZE = 1024;                 //滑动窗口的大小
    std::array<std::vector<char>, WINDOW_SIZE> m_DataBuffer; //滑动窗口的存储空间
    std::vector<bool> m_AllReceivedFlags; //用来记录已经被载入的包序号
    unsigned int m_CheckedReceiveSeq; //在检测丢包时的起点--其左为已经为已经确认接受的连续的包号
    unsigned int m_LastReceivedSeq; //记录最后一个接受的包序号（滑动窗口检测丢包的终点）
    std::vector<unsigned int> m_MissingSeqs; //记录所有缺包的序号

    //接受文件Hash检测失败后，分块检测
    std::vector<bool> m_Verify; //记录Client传入的hash验证序号（一个序号10个包）
    std::vector<std::string> m_HashCodes; //根据索引记录被检验出有问题的hash码
    unsigned int m_NextVerifying;         //下一个检测hash的序号
    std::condition_variable m_CVVerify;   //通知
    std::vector<unsigned int> m_DamagedBlock; //检测出的有问题的块
    // std::condition_variable m_VerifyFinish;   //通知Hash验证的结束

    std::vector<HashRetransmitData> m_HashDatas; //用来存储被检查出Hash有问题的包
    std::condition_variable m_CVRewrite;           //通知重写因为Hash验证失败导致的重写
    unsigned int m_RewriteIndex;                 //下一个检测hash的序号

    //文件持久化操作
    std::fstream m_FileSaveStream;  // 接收文件
    std::ifstream m_VerifyStream;   // 检验Hash码时读取文件数据--VerifyHash()
    unsigned int m_NextExpectedSeq; // 文件持久化时下一个期待的包序号--其左为已保存
    // std::ifstream m_FileUploadStream; //发送文件
    std::mutex m_Mutex;           // 缓冲区操作锁
    std::condition_variable m_CV; // 条件变量
};

class CSession;
//用于发送的File
class FileToSend
{
    friend class FileManagement;
    friend class LogicSystem;

public:
    FileToSend(std::string filepath,
               std::string filename,
               unsigned int filesize,
               unsigned int filetotalpackets,
               std::string filehash,
               std::ifstream file);
    ~FileToSend();
    void SetFileId(short fileid);
    void StartFinishTimer(std::shared_ptr<CSession> session);
    void StopFinishTimer();

private:
    std::string m_FilePath;          //文件在Client中的路径，用于打开文件
    short m_FileId;                  //文件在该Session中的Id
    std::string m_FileName;          //文件名（UTF-8的256字节限定）
    unsigned int m_FileSize;         //文件总长度
    unsigned int m_FileTotalPackets; //文件总包数
    std::string m_FileHash;          //文件验证哈希值

    std::shared_ptr<boost::asio::steady_timer> m_FinishTimer; // 定时器
    bool m_FinishCheck;

    std::ifstream m_FileUploadStream; //发送文件
    std::mutex m_Mutex;               // 缓冲区操作锁
    std::condition_variable m_CV;     // 条件变量
};
