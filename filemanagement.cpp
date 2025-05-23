#include "filemanagement.h"
#include <unordered_map>

FileManagement::FileManagement() {}

FileManagement::~FileManagement() {}

bool FileManagement::AddFile(const std::string &session_uuid,
                             short file_id,
                             std::shared_ptr<FileToReceve> file)
{
    std::lock_guard<std::mutex> lock(m_FileMutex);
    //先找到外层Session的入口
    auto &inner_map = m_Files[session_uuid];
    //插入File到内层map
    auto result = inner_map.emplace(file_id, std::move(file));
    //错误检测
    if (!result.second) {
        // 插入失败（FileId 已存在）
        throw std::runtime_error("FileId already exists in this session");
        return false;
    }
    return true;
}

void FileManagement::AddHashRetransmitDataPacket(const std::string &session_uuid,
                                                 short file_id,
                                                 unsigned int seq,
                                                 const std::vector<char> &data)
{
    std::lock_guard<std::mutex> lock(m_GlobalMutex);
    auto file = findFile(session_uuid, file_id);

    file->m_HashDatas.emplace_back(seq, data);
    file->m_CVRewrite.notify_one();
}

bool FileManagement::AddPacket(const std::string &session_uuid,
                               short file_id,
                               unsigned int seq,
                               const std::vector<char> &data)
{
    std::lock_guard<std::mutex> lock(m_GlobalMutex);
    auto file = findFile(session_uuid, file_id);

    // 1. 检查包序号是否在窗口内
    if ((seq < file->m_WindowStart) || seq >= (file->m_WindowStart + file->WINDOW_SIZE)) {
        return false; // 忽略过期或超前包
    }

    // 2. 计算窗口内的相对位置
    int buffer_pos = seq % file->WINDOW_SIZE;

    // 3. 存储数据并标记位
    file->m_DataBuffer[buffer_pos] = data;
    file->m_AllReceivedFlags[seq] = true;
    file->m_LastReceivedSeq = seq;
    file->SlideWindow();

    file->m_CV.notify_one(); // 通知写入线程
    return true;
}

std::shared_ptr<FileToReceve> FileManagement::findFile(const std::string &session_uuid,
                                                       short file_id)
{
    std::lock_guard<std::mutex> lock(m_FileMutex);

    // 1. 查找外层 Session
    auto outer_it = m_Files.find(session_uuid);
    if (outer_it == m_Files.end()) {
        return nullptr; // Session 不存在
    }

    // 2. 查找内层 FileId
    auto &inner_map = outer_it->second;
    auto inner_it = inner_map.find(file_id);
    if (inner_it == inner_map.end()) {
        return nullptr; // FileId 不存在
    }

    return inner_it->second;
}

bool FileManagement::removeFile(const std::string &session_uuid, short file_id)
{
    std::lock_guard<std::mutex> lock(m_FileMutex);

    auto outer_it = m_Files.find(session_uuid);
    if (outer_it == m_Files.end()) {
        return false; // Session 不存在
    }
    auto &inner_map = outer_it->second; //锁定Session

    auto inner_it = inner_map.find(file_id); //锁定FileId
    if (inner_it == inner_map.end()) {
        return false; // FileId 不存在
    }

    // 删除文件对象（unique_ptr 自动释放内存）
    inner_map.erase(inner_it);

    // 如果内层 Map 为空，清理外层 Map 的 Session 条目
    if (inner_map.empty()) {
        m_Files.erase(outer_it);
    }

    return true;
}
