//优雅退出：Asio版本
#include "cserver.h"
#include "defines.h"
#include "servicepool.h"
#include <iostream>

int main()
{
    try {
        auto pool = ServicePool::GetInstance();
        boost::asio::io_context ioc; //用于监测退出信号
        boost::asio::signal_set
            sigquit(ioc,
                    SIGINT,
                    SIGTERM); //第一个信号是Ctrl+c的强制退出，第二个是右上角的退出按钮对应的信号
        sigquit.async_wait([&ioc, pool](auto, auto) { //两个auto对应两个信号
            ioc.stop();
            pool->Stop();
        });

        CServer server(ioc, PORT);
        ioc.run();

    } catch (std::exception& e) {
        std::cerr << "Server init false :" << e.what() << std::endl;
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
