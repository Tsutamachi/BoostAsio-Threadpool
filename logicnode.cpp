#include "logicnode.h"
#include <iostream>

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecevNode> recevnode)
    : m_Session(session)
    , m_RecevNode(recevnode)
{}

LogicNode::~LogicNode()
{
    // std::cout << "LogicNode Destruct!" << std::endl;
}
