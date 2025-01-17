#pragma once
#include "defines.h"

class MsgNode
{
public:
    MsgNode(short max_len);

    char* m_Data;
    short m_CurLen;
    short m_TotalLen;
};

class RecevNode : public MsgNode
{
public:
    RecevNode(short max_len, short msg_id);
    short m_MsgId;
};

class SendNode : public MsgNode
{
public:
    SendNode(const char* msg, short max_len, short msg_id);
    short m_MsgId;
};
