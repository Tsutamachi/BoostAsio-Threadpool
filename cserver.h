#pragma once
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>

class CSession;
class CServer
{
    friend class CSession;

public:
    CServer(boost::asio::io_context& ioc, short port);
    void ClearSession(std::string uuid);
    void RequestUpload();   //本地：本地文件 ->上传<- 到Data文件夹，并加密保存
    void RequestDownload(); //本地：从Data文件夹中 ->下载<- 解密文件，并持久化为普通文件---这里需要检查Client端对Server端的全线
    void HandleSharedLink(); //还要处理接收分享文件请求，向对方Server ->请求下载<- 的情况--Client端下载和Server端请求下载就分开写吧？

private:
    void StartAccept();
    void HandleAccept(std::shared_ptr<CSession>& new_session,
                      const boost::system::error_code& error);

    boost::asio::io_context& m_ioc;
    short m_port;
    std::mutex m_Mutex;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<CSession>> m_sessions;
};
