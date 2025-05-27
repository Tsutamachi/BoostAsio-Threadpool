#pragma once
#include "const.h"
#include <string>

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short port);
    // 用于服务器的启动
    void Start();

private:
    // 监听器
    tcp::acceptor _acceptor;
    // 上下文
    net::io_context& _ioc;
};
