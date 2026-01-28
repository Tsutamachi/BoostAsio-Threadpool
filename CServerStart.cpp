#include "CServerStart.h"
#include "defines.h"
#include <QThread>

CServerStart::CServerStart(QObject *parent) : QObject(parent)
{
    unsigned short port = SERVERPORT;
    _ioc = std::make_shared<net::io_context>(1);
    _signals = std::make_shared<boost::asio::signal_set>(*_ioc, SIGINT, SIGTERM);

    _signals->async_wait([this](const boost::system::error_code& error, int signal_number) {
        if (!error) _ioc->stop();
    });

    _server = std::make_shared<CServer>(*_ioc, port);
    _server->Start();
    std::cout << "服务器已启动，监听端口: " << port << std::endl;

    _workerThread = std::thread([this]() {
        // std::cout << "工作线程启动，ID: " << std::this_thread::get_id() << std::endl;
        _ioc->run();
        std::cout << "工作线程退出" << std::endl;
    });

    _workerThread.detach();  // 分离线程，让它在后台运行
}

// void CServerStart::Start()
// {
//     unsigned short port = static_cast<unsigned short>(8080);
//     net::io_context ioc{1};
//     boost::asio::signal_set singals(ioc, SIGINT, SIGTERM);
//     singals.async_wait(
//         [&ioc, port](const boost::system::error_code error, int signal_number) {
//             if (error) {
//                 cout << "出现错误!";
//                 return;
//             }
//             ioc.stop();
//         });
//     std::make_shared<CServer>(ioc, port)->Start();
//     cout << "Getserver listen on port:" << port << std::endl;
//     ioc.run();
// }
