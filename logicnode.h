#pragma once
#include <memory>

class CSession;
class RecevNode;
class LogicNode
{
    friend class LogicSystem;

public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecevNode> recevnode);

private:
    std::shared_ptr<CSession> m_Session; //这里是智能”指针“，不会再复制一个Session作为Node的对象
    std::shared_ptr<RecevNode> m_RecevNode;
};
