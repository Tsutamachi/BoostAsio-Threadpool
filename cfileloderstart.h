#pragma once

#include "cserver.h"
#include <QObject>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
class CFileloderStart:public QObject{
    Q_OBJECT
    QML_ELEMENT
public:
    CFileloderStart(QObject *parent = nullptr);
private:
    std::shared_ptr<net::io_context> _ioc;
    std::shared_ptr<boost::asio::signal_set> _signals;
    std::shared_ptr<CServer> _server;
    std::thread _workerThread;
};

