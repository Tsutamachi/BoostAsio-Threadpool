#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;

AsioIOServicePool::AsioIOServicePool(std::size_t size): _ioServices(size),
_works(size), _nextIOService(0) {
    // 方便轮寻
    for (std::size_t i = 0; i < size; ++i) {
        // 将一个上下文传给work
        _works[i] = std::make_unique<WorkGuard>(boost::asio::make_work_guard(_ioServices[i]));
    }
    // 若不写上面那一句话则会直接返回，如果写了则会一直监听

    // 遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}

void AsioIOServicePool::Stop() {
    // 因为仅仅执行work.reset并不能让iocontext从run的状态中退出
    // 当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
    for (auto& work : _works) {
        // 把服务先停止
        work->get_executor().context().stop();
        // 变成一个空指针
        work.reset();
    }

    for (auto& t : _threads) {
        t.join();
    }
}
