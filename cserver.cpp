#include "cserver.h"
#include "HttpConnection.h"
#include <iostream>
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
    : _ioc(ioc)
    // 上下文,端口地址接受p4夏的所有地址
    , _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{}

void CServer::Start()
{
    auto self = shared_from_this();
    // 交给线程池来管理
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
    _acceptor.async_accept(new_con->GetSocket(), [self,new_con](beast::error_code ec) {
        try {
            // 如果出错放弃这个连接,继续监听其他连接
            if (ec) {
                self->Start();
                return;
            }
            // 创建新连接并且创建HTTPConnection类来管理
            new_con->Start();
            // 继续监听
            // std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
            self->Start();
        } catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
    });
}
