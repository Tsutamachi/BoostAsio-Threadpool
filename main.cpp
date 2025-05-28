// //优雅退出：Asio版本
// #include "client.h"
// #include "cserver.h"
// #include "defines.h"
// #include "servicepool.h"
// #include <iostream>
// #include <regex>
// #include <json/value.h>
// #include <json/reader.h>
// // #include <QGuiApplication>
// // #include <QQmlApplicationEngine>
// // #include "cclient.h"


// using namespace boost::asio;
// using ip::tcp;



// int main(int argc, char *argv[])
// {
//     // qmlRegisterType<CClient>("com.example", 1, 0, "CClient");
//     // 注册serverlogin并在点击按钮的时候动态加载
//     // qmlRegisterType<ServerLogin>("com.example", 1, 0, "ServerLogin");
//     // qmlRegisterType<ServerRegister>("com.register", 1, 0, "ServerRegister");
//     // qmlRegisterType<CServerStart>("com.startserver", 1, 0, "CServerStart");
//     // QGuiApplication app(argc, argv);

//     bool flag = true;
//     while (flag) {
//         //选择启动角色--Server or Client
//         std::string role;
//         role.clear();
//         std::cout << "Are you going to log in as Server?(yes->Server  no->Client) ";
//         std::getline(std::cin, role);

//         //Log as Server
//         if (role == "YES" || role == "yes" || role == "Yes"|| role == "y"|| role == "Y") {
//             try {
//                 auto& pool = ServicePool::GetInstance();
//                 boost::asio::io_context ioc; //用于监测退出信号

//                 //这里调用数据库
//                 std::string Servername = "张渝";
//                 auto server = std::make_shared<CServer>(ioc, SERVERPORT,Servername);

//                 boost::asio::signal_set sigquit(
//                             ioc,
//                             SIGINT,
//                             SIGTERM); //第一个信号是Ctrl+c的强制退出，第二个是右上角的退出按钮对应的信号
//                 sigquit.async_wait([&ioc, &pool, &server](auto, auto) { //两个auto对应两个信号
//                     ioc.stop();
//                     pool.Stop();
//                     // server.~CServer();
//                     //    应依赖智能指针自动管理资源。
//                 });

//                 ioc.run();
//                 flag = false;

//             } catch (std::exception& e) {
//                 std::cerr << "Server init false :" << e.what() << std::endl;
//             }
//         }

//         //Log as Client
//         else if (role == "NO" || role == "no" || role == "No"|| role == "n"|| role == "N") {

//             std::cout<<"Are you in the net of your campany?(yes->Inner net  no-> Internet):";
//             std::string net;
//             std::getline(std::cin, net);


//             //Log in the Inner net
//             if(net == "YES" || net == "Yes" || net =="yes" || net == "y" || net == "Y")
//             {
//                 try {
//                     std::string serverip;
//                     std::cout << "Please enter the server ipaddress(v4) you want to link with";
//                     serverip = "127.0.0.1";
//                     // std::getline(std::cin, serverip);

//                     // 使用正则表达式检查 IPv4 地址格式
//                     std::regex ip_regex(
//                                 "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-"
//                                 "4][0-9]|[01]?[0-9][0-9]?)$");

//                     if (!std::regex_match(serverip, ip_regex)) {
//                         std::cerr << "Invalid IPv4 address format." << std::endl;
//                         // 处理错误情况，例如退出或提示用户重新输入
//                         continue;
//                     }

//                     //尝试连接Server
//                     boost::asio::io_context ioc;
//                     // boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::address::from_string(serverip,SERVERPORT);//API更新已经被废弃
//                     boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::make_address(serverip), SERVERPORT);
//                     boost::asio::ip::tcp::socket sock(ioc);
//                     boost::system::error_code error = boost::asio::error::host_not_found;

//                     sock.connect(remote_ep, error);
//                     if (error) {
//                         std::cerr << "connect failed, code is " << error.value() << " error msg is "
//                                   << error.message() << std::endl
//                                   << "Enter anykey to try again." << std::endl;
//                         getchar();
//                         continue;
//                     }

//                     auto client = std::make_shared<Client>(ioc, std::move(sock), CLIENTPORT , true);
//                     boost::asio::signal_set sigquit(ioc, SIGINT, SIGTERM);
//                     sigquit.async_wait([&ioc, &client](auto, auto) {
//                         ioc.stop();
//                         //这里可以做强制退出后的资源处理
//                     });
//                     ioc.run();
//                     flag = false;
//                 }catch (std::exception& e) {
//                     std::cerr << "Client init false :" << e.what() << std::endl;
//                 }
//             }


//             //Log in the Internet
//             else if(net == "No" || net == "No" || net =="no" || net == "n"|| net == "N"){
//                 try{
//                     std::cout << "Please enter the Net Address of Server: ";
//                     std::string host;
//                     std::getline(std::cin, host);

//                     boost::system::error_code error;
//                     boost::asio::io_context ioc;
//                     boost::asio::ip::tcp::resolver resolver(ioc);
//                     boost::asio::ip::tcp::socket sock(ioc);

//                     boost::asio::connect(sock, resolver.resolve(host, std::to_string(LOCALHOST_PORT)), error);

//                     if (error) {
//                         std::cerr << "connect failed, code is " << error.value() << " error msg is "
//                                   << error.message() << std::endl
//                                   << "Enter anykey to try again." << std::endl;
//                         getchar();
//                         continue;
//                     }

//                     auto local_ep = sock.local_endpoint();
//                     auto remote_ep = sock.remote_endpoint();
//                     std::cout << "Connected from " << local_ep.address().to_string() << ":" << local_ep.port()
//                               << " to " << remote_ep.address().to_string() << ":" << remote_ep.port() << std::endl;

//                     // 发送请求,否则Server端无法接收到连接成功信息
//                     // Json::Value re;
//                     // re["email"] = "2405150500@qq.com";
//                     // std::string tar = re.toStyledString();
//                     // std::string request = "POST /post_verifyemail HTTP/1.1\r\n"
//                     //                       "Host: " + host + "\r\n"
//                     //                       "Content-Type: application/json\r\n"
//                     //                       "Content-Length: " + std::to_string(tar.size()) + "\r\n"
//                     //                       "Connection: closed\r\n"
//                     //                       "\r\n"
//                     //                       + tar;

//                     std::string request = "GET /get_login HTTP/1.1\r\n"
//                                           "Host: " + host + "\r\n"
//                                           "Connection: closed\r\n\r\n";

//                     boost::asio::write(sock, boost::asio::buffer(request));

//                     // 读取完整响应
//                     boost::asio::streambuf response;
//                     std::string body;

//                     // 读取直到连接关闭
//                     boost::system::error_code ec;
//                     while (boost::asio::read(sock, response, transfer_all(), ec)) {
//                         body += std::string(buffers_begin(response.data()),
//                                            buffers_end(response.data()));
//                         response.consume(response.size());
//                     }

//                     if (ec && ec != error::eof) throw boost::system::system_error(ec);

//                     // 提取数据体部分（从第一个空行后开始）
//                     size_t pos = body.find("\r\n\r\n");
//                     if (pos != std::string::npos) {
//                         body = body.substr(pos + 4);
//                     }

//                     std::cout <<body << std::endl;

//                     auto client = std::make_shared<Client>(ioc, std::move(sock), LOCALHOST_PORT , false);
//                     boost::asio::signal_set sigquit(ioc, SIGINT, SIGTERM);
//                     sigquit.async_wait([&ioc, &client](auto, auto) {
//                         ioc.stop();
//                         //这里可以做强制退出后的资源处理
//                     });
//                     ioc.run();

//                     flag = false;
//                 }catch (std::exception& e) {
//                     std::cerr << "Client init false :" << e.what() << std::endl;
//                 }
//             }

//             //Invalid input
//             else{
//                 std::cerr << std::endl << "Invalid input! Enter anykey to try again." << std::endl;
//                 getchar();
//             }

//         }
//     }
// }




#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "ServerLogin.h"
#include "ServerRegister.h"
#include "CServerStart.h"
// #include "cclient.h"
int main(int argc, char *argv[])
{
    // qmlRegisterType<CClient>("com.example", 1, 0, "CClient");
    //     // qmlRegisterType<CClient>("com.example", 1, 0, "CClient");
    //     // 注册serverlogin并在点击按钮的时候动态加载
    qmlRegisterType<ServerLogin>("com.example", 1, 0, "ServerLogin");
    qmlRegisterType<ServerRegister>("com.register", 1, 0, "ServerRegister");
    qmlRegisterType<CServerStart>("com.startserver", 1, 0, "CServerStart");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // 使用相对路径或绝对路径加载QML文件
    // 例如，如果Main.qml位于项目目录的qml子目录中，可以使用如下路径：
    // const QUrl url(QStringLiteral("/run/media/root/bccbc01c-f3f5-462e-a7ed-900e1364cf9d/01/Home_page/firstlogin.qml"));
    const QUrl url(QStringLiteral("/root/socket/my/llfc/MyJsonServer//Window.qml"));
    // const QUrl url(QStringLiteral("/run/media/root/bccbc01c-f3f5-462e-a7ed-900e1364cf9d/01/Home_page/Window.qml"));
    QObject::connect(
                &engine,
                &QQmlApplicationEngine::objectCreated,
                &app,
                [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    },
    Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}


