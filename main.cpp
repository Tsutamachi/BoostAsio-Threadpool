//优雅退出：Asio版本
#include "client.h"
#include "cserver.h"
#include "defines.h"
#include "servicepool.h"
#include <iostream>
#include <regex>
#include <json/value.h>
#include <json/reader.h>

int main()
{
    bool flag = true;
    while (flag) {
        //选择启动角色--Server or Client
        std::string role;
        role.clear();
        std::cout << "Are you going to log in as Server?(yes->Server  no->Client) ";
        std::getline(std::cin, role);

        //Log as Server
        if (role == "YES" || role == "yes" || role == "Yes") {
            try {
                auto& pool = ServicePool::GetInstance();
                boost::asio::io_context ioc; //用于监测退出信号
                auto server = std::make_shared<CServer>(ioc, SERVERPORT);

                boost::asio::signal_set sigquit(
                            ioc,
                            SIGINT,
                            SIGTERM); //第一个信号是Ctrl+c的强制退出，第二个是右上角的退出按钮对应的信号
                sigquit.async_wait([&ioc, &pool, &server](auto, auto) { //两个auto对应两个信号
                    ioc.stop();
                    pool.Stop();
                    // server.~CServer();
                });

                ioc.run();
                flag = false;

            } catch (std::exception& e) {
                std::cerr << "Server init false :" << e.what() << std::endl;
            }
        }

        //Log as Client
        else if (role == "NO" || role == "no" || role == "No") {

            std::cout<<"Are you in the net of your campany?(yes->Inner net  no-> Internet):";
            std::string net;
            std::getline(std::cin, net);


            //Log in the Inner net
            if(net == "YES" || net == "Yes" || net =="yes" )
            {
                try {
                    std::string serverip;
                    std::cout << "Please enter the server ipaddress(v4) you want to link with";
                    serverip = "127.0.0.1";
                    // std::getline(std::cin, serverip);

                    // 使用正则表达式检查 IPv4 地址格式
                    std::regex ip_regex(
                                "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-"
                                "4][0-9]|[01]?[0-9][0-9]?)$");

                    if (!std::regex_match(serverip, ip_regex)) {
                        std::cerr << "Invalid IPv4 address format." << std::endl;
                        // 处理错误情况，例如退出或提示用户重新输入
                        continue;
                    }

                    //尝试连接Server
                    boost::asio::io_context ioc;
                    // boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::address::from_string(serverip,SERVERPORT);//API更新已经被废弃
                    boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::make_address(serverip), SERVERPORT);
                    boost::asio::ip::tcp::socket sock(ioc);
                    boost::system::error_code error = boost::asio::error::host_not_found;

                    sock.connect(remote_ep, error);
                    if (error) {
                        std::cerr << "connect failed, code is " << error.value() << " error msg is "
                                  << error.message() << std::endl
                                  << "Enter anykey to try again." << std::endl;
                        getchar();
                        continue;
                    }

                    auto client = std::make_shared<Client>(ioc, std::move(sock), CLIENTPORT , true);
                    boost::asio::signal_set sigquit(ioc, SIGINT, SIGTERM);
                    sigquit.async_wait([&ioc, &client](auto, auto) {
                        ioc.stop();
                        //这里可以做强制退出后的资源处理
                        // client.~Client();
                    });
                    ioc.run();
                    flag = false;
                }catch (std::exception& e) {
                    std::cerr << "Client init false :" << e.what() << std::endl;
                }
            }


            //Log in the Internet
            else if(net == "No" || net == "No" || net =="no"){
                try{
                    std::cout << "Please enter the Net Address of Server: ";
                    std::string host;
                    std::getline(std::cin, host);

                    boost::system::error_code error;
                    boost::asio::io_context ioc;
                    boost::asio::ip::tcp::resolver resolver(ioc);
                    boost::asio::ip::tcp::socket sock(ioc);

                    boost::asio::connect(sock, resolver.resolve(host, std::to_string(LOCALHOST_PORT)), error);

                    if (error) {
                        std::cerr << "connect failed, code is " << error.value() << " error msg is "
                                  << error.message() << std::endl
                                  << "Enter anykey to try again." << std::endl;
                        getchar();
                        continue;
                    }

                    auto local_ep = sock.local_endpoint();
                    auto remote_ep = sock.remote_endpoint();
                    std::cout << "Connected from " << local_ep.address().to_string() << ":" << local_ep.port()
                              << " to " << remote_ep.address().to_string() << ":" << remote_ep.port() << std::endl;

                    // 发送请求,否则Server端无法接收到连接成功信息
                    // Json::Value re;
                    // re["email"] = "2405150500@qq.com";
                    // std::string tar = re.toStyledString();

                    // std::string request = "POST /get_varifycode HTTP/1.1\r\n"
                    //                       "Host: " + host + "\r\n"
                    //                       "Connection: closed\r\n\r\n"
                    //                         + tar;

                    std::string request = "GET /get_test HTTP/1.1\r\n"
                                          "Host: " + host + "\r\n"
                                          "Connection: closed\r\n\r\n";
                    boost::asio::write(sock, boost::asio::buffer(request));

                    std::cout<<"Send First Request Finished!" <<std::endl;

                    auto client = std::make_shared<Client>(ioc, std::move(sock), LOCALHOST_PORT , false);
                    // client->SendTestMsg();
                    boost::asio::signal_set sigquit(ioc, SIGINT, SIGTERM);
                    sigquit.async_wait([&ioc, &client](auto, auto) {
                        ioc.stop();
                        //这里可以做强制退出后的资源处理
                        // client.~Client();
                    });
                    ioc.run();

                    flag = false;
                }catch (std::exception& e) {
                    std::cerr << "Client init false :" << e.what() << std::endl;
                }
            }

            //Invalid input
            else{
                std::cerr << std::endl << "Invalid input! Enter anykey to try again." << std::endl;
                getchar();
            }

        }
    }
}

