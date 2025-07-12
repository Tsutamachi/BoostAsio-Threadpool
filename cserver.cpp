#include "cserver.h"
#include "csession.h"
#include "servicepool.h"
#include "defines.h"
#include "filemanagement.h"

#include <iostream>
#include <boost/filesystem.hpp>
CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
    : m_ioc(ioc)
    // 上下文,端口地址接受p4夏的所有地址
    , m_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    std::cout << "Server start success, listen on port : " << port << std::endl;
}

CServer::CServer(boost::asio::io_context& ioc, short port, std::string name)
    : m_ioc(ioc)
    , m_port(port)
    , m_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , m_ServerName(name)
{
    std::cout << "Server start success, listen on port : " << m_port << std::endl;
    StartAccept();
}

CServer::~CServer() {}
void CServer::ClearSession(std::string uuid)
{
    m_sessions.erase(uuid);
}

void CServer::StartAccept()
{
    auto &iocontext = ServicePool::GetInstance().GetService();
    boost::asio::ip::tcp::socket sock(iocontext);
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(iocontext,
                                                                       this,
                                                                       CSession::Role::Server,
                                                                       std::move(sock));
    m_acceptor
            .async_accept(new_session->GetSocket(),
                          std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::HandleAccept(std::shared_ptr<CSession> &new_session,
                           const boost::system::error_code &error)
{
    if (error) {
        std::cout << "session accept failed, error is " << error.what() << std::endl;
        return;
    }

    std::cout << "\nAccepted connection from: "
              << new_session->GetSocket().remote_endpoint().address().to_string()
              << ":" << new_session->GetSocket().remote_endpoint().port() << std::endl;

    new_session->Start();
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));

    StartAccept();
}

void CServer::Start()
{
    auto self = shared_from_this();
    // 交给线程池来管理
    auto &iocontext = ServicePool::GetInstance().GetService();
    boost::asio::ip::tcp::socket sock(iocontext);
    std::shared_ptr<CSession> new_con = std::make_shared<CSession>(iocontext,
                                                                   this,
                                                                   CSession::Role::Server,
                                                                   std::move(sock)
                                                                   );
    m_acceptor.async_accept(new_con->GetSocket(), [this,self,new_con](boost::beast::error_code ec) {
        try {
            // 如果出错放弃这个连接,继续监听其他连接
            if (ec) {
                self->Start();
                return;
            }
            // 创建新连接并且创建CSession类来管理
            new_con->Start();
            // 继续监听
            // std::make_shared<CSession>(std::move(self->_socket))->Start();
            m_sessions.insert(std::make_pair(new_con->GetUuid(), new_con));

            self->Start();
        } catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
    });
}

std::string CServer::humanReadableSize(uintmax_t bytes) {
    constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    double size = static_cast<double>(bytes);
    int unitIndex = 0;

    while (size >= 1024 && unitIndex < 5) {
        size /= 1024;
        ++unitIndex;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}
uintmax_t CServer::GetNowTransportingSpace(){
    uintmax_t total = 0;
    for (const auto& sessionPair : FileManagement::GetInstance()->m_Files) {
        const std::string& sessionUuid = sessionPair.first;
        for (const auto& filePair : sessionPair.second) {
            short fileId = filePair.first;
            const auto& filePtr = filePair.second;
            total+=filePtr->m_FileSize;
            // std::cout << "Session: " << sessionUuid
            //           << " | File ID: " << fileId << " - ";
            // filePtr->printInfo();
        }
    }
    std::cout<<"正在上传的文件数据量为："<<humanReadableSize(total)<<std::endl;
    return total;
}

uintmax_t CServer::GetHardDiskUseableSpace(){
    try {
        boost::filesystem::space_info si = boost::filesystem::space(StorageHardDisk);

        std::cout << "实际可用空间: " << humanReadableSize(si.available-GetNowTransportingSpace()) << "\n";
        // std::cout << "物理空闲空间: " << humanReadableSize(si.free) << "\n";
        // std::cout << "总空间: " << humanReadableSize(si.capacity) << "\n";

        // double reserved_percent = 100.0 * (si.free - si.available) / si.capacity;
        // std::cout << "系统保留空间: " << reserved_percent << "%\n";

        return 0;
    } catch (const boost::filesystem::filesystem_error& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
