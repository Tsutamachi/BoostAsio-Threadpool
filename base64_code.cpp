#include "base64_code.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdexcept>

std::string base64_encode(const char *data, size_t length)
{
    // 创建 BIO 链：Base64 过滤器 + 内存 BIO
    BIO *b64 = BIO_new(BIO_f_base64());  // 创建 Base64 过滤器
    BIO *bio_mem = BIO_new(BIO_s_mem()); // 创建内存 BIO 用于存储结果
    BIO_push(b64, bio_mem);              // 将 Base64 过滤器连接到内存 BIO

    // 配置：禁用自动换行（根据需求可选）
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    // 写入数据并强制刷新缓冲区
    if (BIO_write(b64, data, static_cast<int>(length)) <= 0) {
        BIO_free_all(b64);
        throw std::runtime_error("BIO_write failed");
    }
    BIO_flush(b64); // 确保所有数据被处理

    // 从内存 BIO 中获取编码结果
    char *encoded_data = nullptr;
    long encoded_len = BIO_get_mem_data(bio_mem, &encoded_data);
    std::string result(encoded_data, encoded_len);

    // 释放资源
    BIO_free_all(b64); // 会连带释放 bio_mem

    return result;
}

std::vector<char> base64_decode(const std::string &encoded_data)
{
    // 创建 BIO 链：Base64 过滤器 + 内存 BIO（输入数据）
    BIO *b64 = BIO_new(BIO_f_base64()); // 创建 Base64 解码过滤器
    BIO *bio_mem = BIO_new_mem_buf(encoded_data.data(), static_cast<int>(encoded_data.size()));
    BIO_push(b64, bio_mem); // 将过滤器连接到输入内存 BIO

    // 配置：禁用自动换行（与编码设置一致）
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    // 读取解码后的数据到 std::vector<char>
    std::vector<char> decoded_data;
    char buffer[512]; // 缓冲区大小可根据需求调整
    int len;

    while ((len = BIO_read(b64, buffer, sizeof(buffer))) > 0) {
        // 将缓冲区数据追加到 vector
        decoded_data.insert(decoded_data.end(), buffer, buffer + len);
    }

    // 检查是否读取失败（len < 0 表示错误）
    if (len < 0) {
        BIO_free_all(b64);
        throw std::runtime_error("BIO_read failed during Base64 decoding");
    }

    // 释放资源（BIO链）
    BIO_free_all(b64);

    return decoded_data;
}
