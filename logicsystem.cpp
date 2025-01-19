#include "logicsystem.h"
#include "csession.h"
#include <json-c/json.h>
#include <json/reader.h>
#include <json/value.h>

LogicSystem::LogicSystem()
    : m_stop(false)
{
    RegisterCallBacks();
    m_WorkerThread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    m_stop = true;
    m_Consume.notify_one(); //最后唤醒一次线程处理剩余没发送的信息
    m_WorkerThread.join();  //等待线程结束再回收资源
}

void LogicSystem::PostMesgToQue(std::shared_ptr<LogicNode> node)
{
    std::unique_lock<std::mutex> unique_lk(m_Mutex);
    m_MegQue.push(node);

    //队列如果是从0->1,则发送通知信号
    if (m_MegQue.size() == 1) {
        unique_lk.unlock();
        m_Consume.notify_one();
    }
}

void LogicSystem::RegisterCallBacks()
{
    m_FunCallBacks[MEG_HELLO_WORLD] = std::bind(&LogicSystem::HandleHelloWorld,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    //后续可拓展消息id对应的回调函数
}

void LogicSystem::HandleHelloWorld(std::shared_ptr<CSession> session,
                                   const short &msg_id,
                                   const std::string &msg_data)
{ //echo模式，回显回到Client端
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,
                 root); //因为这里传入的参数是string,所以不需要通过buffer首地址和长度来构造string

    std::cout << "recevie msg id  is " << root["id"].asInt() << std::endl
              << "       msg data is " << root["data"].asString() << std::endl;

    std::string return_str = root.toStyledString();
    session->Send(return_str.data(), return_str.size(), (short) root["id"].asInt());
}

void LogicSystem::DealMsg()
{
    while (true) {
        std::unique_lock<std::mutex> unique_lk(m_Mutex);

        //同时满足 1、队列为空 2、没有停止传输(正在传输) 则用条件变量unique_lk阻塞等待，并释放锁
        while (m_MegQue.empty() && !m_stop) {
            m_Consume.wait(unique_lk);
        }

        //停止了传输 && 队列中仍有信息
        if (m_stop) {
            while (!m_MegQue.empty()) {
                auto meg_node = m_MegQue.front();
                std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

                auto call_back_iter = m_FunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
                if (call_back_iter
                    == m_FunCallBacks
                           .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
                    m_MegQue.pop();
                    continue;
                }

                //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
                call_back_iter->second(meg_node->m_Session,
                                       meg_node->m_RecevNode->m_MsgId,
                                       std::string(meg_node->m_RecevNode->m_Data,
                                                   meg_node->m_RecevNode
                                                       ->m_CurLen)); //深度拷贝RecevNode中的Data
                m_MegQue.pop();
            }
            break;
        }

        //正在传输
        auto meg_node = m_MegQue.front();
        std::cout << "RecevMeg Id is: " << meg_node->m_RecevNode->m_MsgId << std::endl;

        auto call_back_iter = m_FunCallBacks.find(meg_node->m_RecevNode->m_MsgId);
        if (call_back_iter
            == m_FunCallBacks
                   .end()) { //在所有注册的回调函数中，没有与当前消息对应的回调函数，即当前MsgId违法
            m_MegQue.pop();
            continue;
        }

        //second函数是在调用call_back_iter对应的回调函数，括号内是回调函数的参数
        call_back_iter
            ->second(meg_node->m_Session,
                     meg_node->m_RecevNode->m_MsgId,
                     std::string(meg_node->m_RecevNode->m_Data,
                                 meg_node->m_RecevNode->m_CurLen)); //深度拷贝RecevNode中的Data
        m_MegQue.pop();
    }
}
