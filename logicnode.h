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
    std::shared_ptr<CSession> m_Session;
    std::shared_ptr<RecevNode> m_RecevNode;
};
