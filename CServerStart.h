#pragma once
#include "cserver.h"
#include <QObject>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
class CServerStart:public QObject{
    Q_OBJECT
    QML_ELEMENT
public:
    CServerStart(QObject *parent = nullptr);
    Q_INVOKABLE void Start();
private:
    std::shared_ptr<net::io_context> _ioc;
    std::shared_ptr<boost::asio::signal_set> _signals;
    std::shared_ptr<CServer> _server;
    std::thread _workerThread;
};
