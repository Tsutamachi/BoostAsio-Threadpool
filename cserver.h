#pragma once
#include "defines.h"
#include "csession.h"
#include <map>
#include <string>

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, short port, std::string name);
    CServer(boost::asio::io_context& ioc, short port);
    ~CServer();
    void ClearSession(std::string uuid);
    void RequestUpload();   //本地：本地文件 ->上传<- 到Data文件夹，并加密保存
    void RequestDownload(); //本地：从Data文件夹中 ->下载<- 解密文件，并持久化为普通文件---这里需要检查Client端对Server端的全线
    void HandleSharedLink(); //还要处理接收分享文件请求，向对方Server ->请求下载<- 的情况--Client端下载和Server端请求下载就分开写吧？


    CServer(boost::asio::io_context& ioc, unsigned short port);
    // 用于服务器的启动
    void Start();

    uintmax_t GetHardDiskUseableSpace();//获得当前硬盘中的可用空间（减去正在传输的文件大小）

    std::string m_ServerName; //Server登陆时的账号

private:
    void StartAccept();
    void HandleAccept(std::shared_ptr<CSession>& new_session,
                      const boost::system::error_code& error);

    uintmax_t GetNowTransportingSpace();
    std::string humanReadableSize(uintmax_t bytes);


    boost::asio::io_context& m_ioc;
    short m_port;
    std::mutex m_Mutex;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::map<std::string, std::shared_ptr<CSession>> m_sessions;

    int m_ServerId;           //DB中自动分配的Id
    //需要修改密码和口令的时候，再从数据库中提取？

};
