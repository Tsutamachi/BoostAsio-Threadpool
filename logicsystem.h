#pragma once
#include "Singleton.h"
#include "defines.h"
#include "logicnode.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

typedef std::function<
    void(std::shared_ptr<CSession> session, const short &msg_id, const std::string &msg_data)>
    FunCallBack;

class CSession;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class CSession;
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();
    void PostMesgToQue(std::shared_ptr<LogicNode> node);

private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks(); //注册：消息id <-> 回调函数
    void HandleHelloWorld(std::shared_ptr<CSession> session,
                          const short &msg_id,
                          const std::string &msg_data); //消息id为helloworld的回调函数

    std::thread m_WorkerThread;
    std::queue<std::shared_ptr<LogicNode>> m_MegQue;
    std::mutex m_Mutex;
    std::map<short, FunCallBack> m_FunCallBacks;
    std::condition_variable m_Consume; //能在某个条件成立之前 阻塞线程 并在条件成立时 唤醒线程的机制
    bool m_stop;
};
