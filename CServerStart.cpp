#include "CServerStart.h"
#include <QThread>

CServerStart::CServerStart(QObject *parent) : QObject(parent)
{
    unsigned short port = 8080;
    _ioc = std::make_shared<net::io_context>(1);
    _signals = std::make_shared<boost::asio::signal_set>(*_ioc, SIGINT, SIGTERM);

    _signals->async_wait([this](const boost::system::error_code& error, int signal_number) {
        if (!error) _ioc->stop();
    });

    _server = std::make_shared<CServer>(*_ioc, port);
    _server->Start();

    _workerThread = std::thread([this]() {
        _ioc->run();
    });

    // 使用 join() 而非 detach()，在析构函数中等待线程结束
    // _workerThread.detach();
}





void CServerStart::Start()
{
    unsigned short port = static_cast<unsigned short>(8080);
    net::io_context ioc{1};
    boost::asio::signal_set singals(ioc, SIGINT, SIGTERM);
    singals.async_wait(
        [&ioc, port](const boost::system::error_code error, int signal_number) {
            if (error) {
                cout << "出现错误!";
                return;
            }
            ioc.stop();
        });
    std::make_shared<CServer>(ioc, port)->Start();
    cout << "Getserver listen on port:" << port << std::endl;
    ioc.run();
}
