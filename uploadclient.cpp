#include "uploadclient.h"
Uploadclient::Uploadclient(QObject *parent):QObject(parent)
{
    _workerThread = std::thread([this]() {
        boost::asio::io_context ioc;
        std::string ipaddr=m_ip.toStdString();
        boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::make_address(ipaddr), SERVERPORT);
        boost::asio::ip::tcp::socket sock(ioc);
        boost::system::error_code error = boost::asio::error::host_not_found;
        sock.connect(remote_ep, error);
        if (error) {
            std::cerr << "connect failed, code is " << error.value() << " error msg is "
                      << error.message() << std::endl
                      << "Enter anykey to try again." << std::endl;
            getchar();
        }

        _client=std::make_shared<Client>(ioc, std::move(sock), CLIENTPORT , true);
        std::cout << "工作线程启动，ID: " << std::this_thread::get_id() << std::endl;
        ioc.run();
        std::cout << "工作线程退出" << std::endl;
    });
    _workerThread.detach();  // 分离线程，让它在后台运行

}

void  Uploadclient::startUpload(QString filepath)
{
    std::string filepath1=filepath.toStdString();
    _client->RequestUpload(filepath1);

}

QString Uploadclient::ip() const
{
    return m_ip;

}

void Uploadclient::setIp(QString &ip)
{
    m_ip=ip;
}
