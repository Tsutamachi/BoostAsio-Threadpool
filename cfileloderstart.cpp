#include "cfileloderstart.h"

CFileloderStart::CFileloderStart(QObject *parent): QObject(parent)
{
    unsigned short port = 1220;
    _ioc = std::make_shared<net::io_context>(1);
    _signals = std::make_shared<boost::asio::signal_set>(*_ioc, SIGINT, SIGTERM);

    _signals->async_wait([this](const boost::system::error_code& error, int signal_number) {
        if (!error) _ioc->stop();
    });
    std::string name="aaa";
    _server = std::make_shared<CServer>(*_ioc, port,name);
    std::cout << "服务器已启动，监听端口: " << port << std::endl;

    _workerThread = std::thread([this]() {
        // std::cout << "工作线程启动，ID: " << std::this_thread::get_id() << std::endl;
        _ioc->run();
        std::cout << "工作线程退出" << std::endl;
    });

    _workerThread.detach();  // 分离线程，让它在后台运行
}
