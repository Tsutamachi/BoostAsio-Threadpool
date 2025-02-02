#include "client.h"

Client::Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket& socket, short port)
    : m_Ioc(ioc)
    , m_Socket(socket)
    , m_port(port)
{
    std::cout << "Client start success, listen on port : " << m_port << std::endl;
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(m_Ioc, this);
}

void Client::AddFileToSend() {}

FileToSend Client::FindFileToSend(short fileid)
{
    //从m_FilesToSend中通过fileid索引找到FileToSend
}
