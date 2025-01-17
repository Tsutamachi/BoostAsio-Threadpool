#include "msgnode.h"
#include <boost/asio.hpp>
#include <iostream>
MsgNode::MsgNode(short max_len)
    : m_TotalLen(max_len)
    , m_CurLen(0)
{
    m_Data = new char[m_TotalLen + 1](); //加上()能直接初始化内存（减少出错）；没有则只分配，未初始化
    m_Data[m_TotalLen] = '\0';
}

RecevNode::RecevNode(short max_len, short msg_id)
    : MsgNode(max_len + HEAD_TOTLE_LEN)
    , m_MsgId(msg_id)
{}

SendNode::SendNode(const char *msg, short max_len, short msg_id)
    : MsgNode(max_len + HEAD_TOTLE_LEN)
    , m_MsgId(msg_id)
{
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(m_Data, &msg_id_host, HEAD_ID_LEN);

    short maxlen = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(m_Data + HEAD_ID_LEN, &maxlen, HEAD_DATA_LEN);

    memcpy(m_Data + HEAD_TOTLE_LEN, msg, max_len); //将传入到Node的msg复制到data中

    std::cout << "SendNode Construct!" /*<< *m_Data */ << std::endl;
    std::cout << "m_TotalLen= " << m_TotalLen << std::endl;
    std::cout << "m_MsgId= " << m_MsgId << std::endl;
    std::cout << "m_Data:" << std::endl;
    for (int i = HEAD_TOTLE_LEN; i < m_TotalLen + HEAD_TOTLE_LEN; ++i) {
        // 只打印实际的消息内容
        std::cout << m_Data[i];
    }
    std::cout << std::endl;
}
