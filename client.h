#pragma once
#include "file.h"
#include <memory>
#include <vector>

class Client
{
public:
    Client();
    void AddFileToSend();

private:
    std::vector<std::unique_ptr<FileToSend>> m_FilesToSend;
    std::unique_ptr<FileToSend>
        m_TempFile; //为了解决在没有FileId前，不能对应到m_FilesToSend进行存储的问题。
};
