#include "client.h"
#include "csession.h"
#include "logicsystem.h"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <json/json.h>
#include <json/value.h>

using namespace boost::asio::ip;

Client::Client(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket& socket, short port)
    : m_Ioc(ioc)
    , m_Socket(std::move(socket))
    , m_port(port)
    , m_Session(
          std::make_shared<CSession>(ioc,
                                     this,
                                     CSession::Role::Client,
                                     std::move(m_Socket))) //只有一个Session,可以直接初始化列表了
{
    std::cout << "Client start success, listen on port : " << m_port << std::endl;
    m_Session->Start();

    Greating();
}

void Client::Greating()
{
    std::cout << "Please select the function:" << std::endl
              << "-------------------------------------------------------------------------"
              << std::endl
              << "1、EchoTestOnce                      2、EchoTestAll                       "
              << std::endl
              << "3、RequestUpload                     4、RequestDownLoad                   "
              << std::endl
              << "-------------------------------------------------------------------------"
              << std::endl
              << std::endl;
    int choose;
    while (!(std::cin >> choose)) {
        std::cout << "Error input! Please enter a number." << std::endl;
        std::cin.clear();                                                   // 清除错误标志
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余的输入
    }

    switch (choose) {
    case 1: {
        Test1();
        break;
    }
    case 2: {
        EchoTest();
        break;
    }
    case 3: {
        RequestUpload();
        break;
    }
    case 4: {
        RequestDownload();
        break;
    }
    default: {
        std::cout << "Error input!" << std::endl;
    }
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余的输入
}

void Client::HandleSessionClose()
{
    //在这里可以添加Client的资源回收和各种文件的持久化保存，如断点续传的包数

    m_Session.reset(); //清理Session资源
    m_Socket.close();  //清理Socket资源
}

void Client::AddFileToSend(std::shared_ptr<FileToSend> tempfile, short fileid)
{
    // m_FilesToSend[fileid] = tempfile;
    if (fileid >= 0 && fileid < m_FilesToSend.size()) {
        // 如果 fileid 是有效的索引，则替换对应位置的元素
        m_FilesToSend[fileid] = tempfile;
    } else if (fileid == m_FilesToSend.size()) {
        // 如果 fileid 等于当前向量的 size，则添加到向量末尾
        m_FilesToSend.push_back(tempfile);
    } else {
        // fileid 无效，可以抛出异常或进行其他错误处理
        throw std::out_of_range("Invalid file ID");
    }
}

std::shared_ptr<FileToSend> Client::FindFileToSend(short fileid)
{
    //从m_FilesToSend中通过fileid索引找到FileToSend
    if (m_FilesToSend[fileid])
        return m_FilesToSend[fileid];
    else
        return nullptr;
}

void Client::RemoveFile(short fileid)
{
    // erase会自动调用析构函数
    // 检查索引是否有效
    if (fileid < m_FilesToSend.size()) {
        m_FilesToSend.erase(m_FilesToSend.begin() + fileid);
    } else {
        std::cerr << "RemoveFile Index out of range" << std::endl;
    }
    std::cout << "FileId :" << fileid << " Removed!" << std::endl << std::endl << std::endl;
    Greating();
}

void Client::RequestUpload()
{
    //指定需要上传的文件的路径（这里可以设置成一次选择多个文件（limit 5），文件路径存入缓存队列中，LogicSystem从缓存队列中取数据）
    std::string filepath;
    std::cout << "Please enter the path of file you want to upload: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清除输入缓冲区的换行符
    std::getline(std::cin, filepath);

    Json::Value Msg;
    Msg["filepath"] = filepath;
    std::string target = Msg.toStyledString();

    //直接发送请求信息到LogicSystem中,不经过Session中转
    std::shared_ptr RecevMsgNode = std::make_shared<RecevNode>(target.size(), FileUploadRequest);
    memcpy(RecevMsgNode->m_Data, target.data(), target.size());
    LogicSystem::GetInstance()->PostMesgToQue(make_shared<LogicNode>(m_Session, RecevMsgNode));
}

void Client::RequestDownload()
{
    //从用户空间中指定文件，把在Client端中的路径，传入到Server中，Server查表找到对应被加密的文件
    //todo:只能在确定用户空间文件管理表后才能写
}

void Client::EchoTest()
{
    auto start = std::chrono::high_resolution_clock::now(); // 获取开始时间
    for (int i = 0; i < 100; i++) {
        vec_threads.emplace_back([this]() -> void {
            try {
                boost::asio::io_context ioc;
                tcp::endpoint remote_ep(address::from_string("127.0.0.1"), PORT);
                tcp::socket sock(ioc);
                boost::system::error_code error = boost::asio::error::host_not_found;
                sock.connect(remote_ep, error);
                if (error) {
                    std::cout << "connect failed, code is " << error.value() << " error msg is "
                              << error.message();
                    return;
                }
                for (int j = 0; j < 500; ++j) {
                    Json::Value root;
                    root["id"] = Test;
                    root["data"] = "hello world" + std::to_string(j);
                    // std::cout << "Root[data]: " << root["data"] << std::endl;
                    std::string target = root.toStyledString();

                    //直接发送请求信息到LogicSystem中,不经过Session中转
                    std::shared_ptr<RecevNode> RecevMsgNode
                        = std::make_shared<RecevNode>(target.size(), Test);
                    memcpy(RecevMsgNode->m_Data, target.data(), target.size());

                    LogicSystem::GetInstance()->PostMesgToQue(
                        make_shared<LogicNode>(m_Session, RecevMsgNode));

                }
            } catch (std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto& t : vec_threads) {
        t.join();
    }
    // 执行一些需要计时的操作
    auto end = std::chrono::high_resolution_clock::now(); // 获取结束时间

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        end - start); // 计算时间差，单位为微秒
    std::cout << "Time spent: " << duration.count() << " seconds." << std::endl;
    getchar();
}

void Client::Test1()
{
    Json::Value Msg;
    Msg["id"] = Test;
    Msg["data"] = "hello world";
    std::string target = Msg.toStyledString();

    //直接发送请求信息到LogicSystem中,不经过Session中转
    std::shared_ptr<RecevNode> RecevMsgNode = std::make_shared<RecevNode>(target.size(), Test);
    memcpy(RecevMsgNode->m_Data, target.data(), target.size());

    LogicSystem::GetInstance()->PostMesgToQue(make_shared<LogicNode>(m_Session, RecevMsgNode));
}
