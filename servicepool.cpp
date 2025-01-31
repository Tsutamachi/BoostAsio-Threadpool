#include "servicepool.h"
#include <iostream>

//每一个WorkPtr中都有一个io_Context,每个Context中都会运行size个线程来进行实际工作。
ServicePool::ServicePool()
    : m_Services(size)
    , m_Works(size)
    , m_NextServiceIndex(0)
{
    for (std::size_t i = 0; i < size; ++i) {
        m_Works[i] = std::unique_ptr<boost::asio::io_context::work>(
            new boost::asio::io_context::work(m_Services[i]));
        //也可以new出智能指针后，用std::move的方式移给m_Works[i]。m_Works内数据结构也是智能指针，不能当左值被赋值。
    }

    for (std::size_t i = 0; i < m_Services.size(); ++i) {
        //emplace_back 通常更高效，因为它直接在容器管理的内存空间中构造元素，从而避免了额外的复制或移动操作。
        m_Threads.emplace_back([this, i]() {
            m_Services[i].run();
        }); //这里emplace_back的参数是调用构造函数后的回调函数
    }
}

ServicePool::~ServicePool()
{
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context &ServicePool::GetService()
{
    std::unique_lock<std::mutex> unique_lk(m_Mutex);
    auto &service = m_Services[m_NextServiceIndex++];
    if (m_NextServiceIndex == m_Services.size())
        m_NextServiceIndex = 0;
    return service;
}

ServicePool &ServicePool::GetInstance()
{
    static ServicePool instance;
    return instance;
}

void ServicePool::Stop()
{
    //因为仅仅执行work.reset并不能让iocontext从run的状态中退出
    //当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
    for (auto &work : m_Works) {
        //把服务先停止
        work->get_io_context().stop();
        work.reset(); //释放智能指针
    }

    for (auto &t : m_Threads) {
        t.join(); //等待所有线程退出
    }
}
