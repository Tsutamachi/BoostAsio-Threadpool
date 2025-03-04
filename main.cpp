//优雅退出：Asio版本
#include "client.h"
#include "cserver.h"
#include "defines.h"
#include "servicepool.h"
#include <iostream>
#include <regex>

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

                boost::asio::signal_set sigquit(
                    ioc,
                    SIGINT,
                    SIGTERM); //第一个信号是Ctrl+c的强制退出，第二个是右上角的退出按钮对应的信号
                sigquit.async_wait([&ioc, &pool](auto, auto) { //两个auto对应两个信号
                    ioc.stop();
                    pool.Stop();
                });

                CServer server(ioc, PORT);
                ioc.run();
                flag = false;

            } catch (std::exception& e) {
                std::cerr << "Server init false :" << e.what() << std::endl;
            }
        }

        //Log as Client
        else if (role == "NO" || role == "no" || role == "No") {
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
                boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::address::from_string(
                                                             serverip),
                                                         PORT);
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

                boost::asio::signal_set sigquit(ioc, SIGINT, SIGTERM);
                sigquit.async_wait([&ioc](auto, auto) {
                    ioc.stop();
                    //这里可以做强制退出后的资源处理
                });
                Client client(ioc, sock, PORT);
                ioc.run();
                flag = false;

            } catch (std::exception& e) {
                std::cerr << "Client init false :" << e.what() << std::endl;
            }
        }

        //Input error
        else {
            std::cerr << std::endl << "Invalid input! Enter anykey to try again." << std::endl;
            getchar();
        }
    }
}

//优雅退出：通用版本
// #include "Singleton.h"
// #include "cserver.h"
// #include "logicsystem.h"
// #include <csignal>
// #include <iostream>
// #include <mutex>
// #include <thread>
// using namespace std;
// bool bstop = false;
// std::condition_variable cond_quit;
// std::mutex mutex_quit;
// void sig_handler(int sig)
// {
//     if (sig == SIGINT || sig == SIGTERM) {
//         std::unique_lock<std::mutex> lock_quit(mutex_quit);
//         bstop = true;
//         lock_quit.unlock();
//         cond_quit.notify_one();
//     }
// }

// int main()
// {
//     try {
//         boost::asio::io_context io_context;
//         std::thread net_work_thread([&io_context] { //子进程执行网络程序
//             CServer s(io_context, 10086);
//             io_context.run();
//         });
//         signal(SIGINT, sig_handler); //为信号注册回调函数
//         signal(SIGTERM, sig_handler);
//         while (!bstop) {
//             std::unique_lock<std::mutex> lock_quit(mutex_quit);
//             cond_quit.wait(lock_quit); //会监听cond_quit.notify_，监听到了，就解锁对应的mutex条件变量
//         }
//         io_context.stop();
//         net_work_thread.join(); //等待网络子线程退出后，该线程才会退出

//     } catch (std::exception& e) {
//         std::cerr << "Exception: " << e.what() << endl;
//     }
// }
