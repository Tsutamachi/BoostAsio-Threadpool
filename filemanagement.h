#pragma once
#include "Singleton.h"
#include "file.h"
#include <memory>
#include <unordered_map>

class FileManagement : public Singleton<FileManagement>
{
    friend class Singleton<FileManagement>;
    friend class File;

public:
    ~FileManagement();

    // 创建新的传输任务
    short CreateTransfer(const std::string& session_uuid,
                         const std::string& filename,
                         int total_packets);

    // 添加数据包到指定传输任务
    bool AddPacket(const std::string& session_uuid,
                   short file_id,
                   unsigned int seq,
                   const std::vector<char>& data);

    bool AddFile(const std::string& session_uuid, short file_id, std::unique_ptr<File> file);

    File* findFile(const std::string& session_uuid, short file_id);

    bool removeFile(const std::string& session_uuid, short file_id);

private:
    FileManagement();

    //注意：什么时候需要清理map中的空闲资源？1、Session关闭 2、超时 3、File验证完毕完成传输后
    std::unordered_map<std::string,                                       //SessionUuid
                       std::unordered_map<short,                          //FIleId
                                          std::unique_ptr<FileToReceve>>> //File指针
        m_Files;
    std::mutex m_GlobalMutex;
    std::atomic<short> m_NextFileId{0};
};
