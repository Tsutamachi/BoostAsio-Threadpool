#pragma once
//QML中Client用于调用文件上传
#include "client.h"
#include <QObject>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
class Uploadclient:public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString ip READ ip WRITE setIp NOTIFY ipChange FINAL)
public:
    Uploadclient(QObject *parent = nullptr);
    Q_INVOKABLE void startUpload(QString filepath);
    QString ip() const;
    void setIp(QString &ip);

signals:
    void ipChange();
private:
    std::shared_ptr<net::io_context> _ioc;
    std::shared_ptr<boost::asio::signal_set> _signals;
    std::shared_ptr<Client> _client;
    std::thread _workerThread;
    QString m_ip;

};

