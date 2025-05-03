// filetransfer.h
#pragma once
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include "defines.h"

class FileTransfer {
public:
    // 通用文件发送函数（用于Server响应下载或Client主动上传）
    static void SendFile(boost::asio::ip::tcp::socket& socket,
                        const std::string& filepath) {
        try {
            std::ifstream file(filepath, std::ios::binary);
            if (!file) {
                std::cerr << "File not found: " << filepath << std::endl;
                return;
            }

            char buffer[FILE_DATA_LEN];
            while (file.is_open()) {
                file.read(buffer, sizeof(buffer));
                size_t bytes_read = file.gcount();
                boost::asio::async_write(socket,
                                  boost::asio::buffer(buffer, bytes_read));
            }
        } catch (std::exception& e) {
            std::cerr << "SendFile Error: " << e.what() << std::endl;
        }
    }

    // 通用文件接收函数（用于Server处理上传或Client处理下载）
    static void ReceiveFile(boost::asio::ip::tcp::socket& socket,
                           const std::string& savepath) {
        try {
            std::ofstream file(savepath, std::ios::binary);
            if (!file) {
                std::cerr << "Cannot create file: " << savepath << std::endl;
                return;
            }

            char buffer[FILE_DATA_LEN];
            boost::system::error_code ec;
            while (true) {
                size_t len = socket.read_some(boost::asio::buffer(buffer), ec);
                if (ec == boost::asio::error::eof) break; // 连接正常关闭
                else if (ec) throw boost::system::system_error(ec);
                file.write(buffer, len);
            }
        } catch (std::exception& e) {
            std::cerr << "ReceiveFile Error: " << e.what() << std::endl;
        }
    }
};
