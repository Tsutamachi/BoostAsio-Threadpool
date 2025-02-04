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

    std::cout << "Please select the function:" << std::endl
              << "-------------------------------------------------------------------------"
              << std::endl
              << "1、EchoTest                          2、UploadFile                        "
              << std::endl
              << "-------------------------------------------------------------------------"
              << std::endl
              << std::endl;
    int choose;
    std::cin >> choose;

    switch (choose) {
    case 1: {
        EchoTest();
        break;
    }
    case 2: {
        RequestUpload();
        break;
    }
    case 3: {
        Test1();
        break;
    }
    }
}

void Client::HandleSessionClose()
{
    //在这里可以添加Client的资源回收和各种文件的持久化保存，如断点续传的包数

    m_Session.reset(); //清理Session资源
    m_Socket.close();  //清理Socket资源
}

void Client::AddFileToSend(std::unique_ptr<FileToSend> tempfile, short fileid)
{
    m_FilesToSend[fileid] = std::move(tempfile);
}

std::unique_ptr<FileToSend>& Client::FindFileToSend(short fileid)
{
    //从m_FilesToSend中通过fileid索引找到FileToSend
    return m_FilesToSend[fileid];
}

void Client::RequestUpload()
{
    //指定需要上传的文件的路径（这里可以设置成一次选择多个文件（limit 5），文件路径存入缓存队列中，LogicSystem从缓存队列中取数据）
    std::string filepath;
    std::cout << "Please enter the path of file you want to upload: ";
    std::getline(std::cin, filepath);

    Json::Value Msg;
    Msg["filepath"] = filepath;
    std::string target = Msg.toStyledString();

    //直接发送请求信息到LogicSystem中,不经过Session中转
    std::shared_ptr RecevMsgNode = std::make_shared<RecevNode>(target.size(), FileUploadRequest);
    memcpy(RecevMsgNode->m_Data, &target, target.size());
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

                    // std::string request = root.toStyledString();
                    // size_t request_length = request.length();
                    // char send_data[MAX_LENGTH] = {0};
                    // int msgid = 1001;
                    // int msgid_host = boost::asio::detail::socket_ops::host_to_network_short(msgid);
                    // memcpy(send_data, &msgid_host, 2);
                    // //转为网络字节序
                    // int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(
                    //     request_length);
                    // memcpy(send_data + 2, &request_host_length, 2);
                    // memcpy(send_data + 4, request.c_str(), request_length);
                    // boost::asio::write(sock, boost::asio::buffer(send_data, request_length + 4));

                    // std::cout << "begin to receive..." << std::endl;
                    // char reply_head[HEAD_TOTAL_LEN];
                    // size_t reply_length = boost::asio::read(sock,
                    //                                         boost::asio::buffer(reply_head,
                    //                                                             HEAD_TOTAL_LEN));

                    // msgid = 0;
                    // memcpy(&msgid, reply_head, HEAD_ID_LEN);
                    // short msglen = 0;
                    // memcpy(&msglen, reply_head + 2, HEAD_DATA_LEN);
                    // //转为本地字节序
                    // msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
                    // msgid = boost::asio::detail::socket_ops::network_to_host_short(msgid);
                    // char msg[MAX_LENGTH] = {0};
                    // size_t msg_length = boost::asio::read(sock, boost::asio::buffer(msg, msglen));
                    // Json::Reader reader;
                    // reader.parse(std::string(msg, msg_length), root);
                    // std::cout << "msg id is " << root["id"] << " msg is " << root["data"]
                    //           << std::endl;
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
