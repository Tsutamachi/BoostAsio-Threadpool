#include "hashmd5.h"
#include <fstream>
#include <iomanip>
#include <openssl/md5.h>
#include <sstream>

std::string CalculateFileHash(const std::string &filepath)
{
    // 获取文件大小
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    size_t file_size = file.tellg();
    file.close();

    // 根据文件大小选择验证方式
    if (file_size < 10 * 1024 * 1024) { // 小于 10 MB
        return CalculateSmallFileHash(filepath);
    } else { // 大于等于 10 MB
        return CalculateLargeFileHash(filepath);
    }
}

std::string CalculateSmallFileHash(const std::string &filepath)
{
    // 一次性读取文件到内存
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::string file_content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

    // 计算哈希值
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char *>(file_content.data()), file_content.size(), hash);

    // 转换为十六进制字符串
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

std::string CalculateLargeFileHash(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[1024 * 1024]; // 1MB 缓冲区
    while (file.read(buffer, sizeof(buffer))) {
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &md5Context);

    // 转换为十六进制字符串
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

bool VerifyFileHash(const std::string &filepath, const std::string clienthash)
{
    std::string serverhash = CalculateFileHash(filepath);
    if (serverhash == clienthash)
        return true;
    else
        return false;
}
