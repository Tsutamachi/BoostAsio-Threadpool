#pragma once
#include "defines.h"
#include <algorithm>
#include <string.h>

class MsgNode
{
public:
    MsgNode(short max_len);

    void Clear()
    {
        memset(m_Data, 0, m_TotalLen);
        m_CurLen = 0;
    }

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
