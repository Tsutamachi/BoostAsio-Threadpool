#pragma once
#include "file.h"
#include <memory>
#include <vector>

class Client
{
public:
    Client();
    void AddFileToSend();
    FileToSend FindFileToSend(short fileid);

private:
    std::vector<std::unique_ptr<FileToSend>> m_FilesToSend;
    //TempFile为了解决在没有FileId前，不能对应到m_FilesToSend进行存储的问题。
    std::unique_ptr<FileToSend> m_TempFile;
};
