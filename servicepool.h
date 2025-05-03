#pragma once
// #include "Singleton.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <boost/asio/executor_work_guard.hpp>

class ServicePool /*: public Singleton<ServicePool>*/
{
    // friend class Singleton<ServicePool>;

public:
    // using WorkPtr = std::unique_ptr<boost::asio::io_context::work>;//API更新后被废弃，使用下面的进行替代
    using WorkPtr = std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>;
    ServicePool(const ServicePool &) = delete;
    ServicePool &operator=(const ServicePool &) = delete;

    boost::asio::io_context &GetService();
    static ServicePool &GetInstance();

    void Stop();
    ~ServicePool();

private:
    ServicePool();

    std::size_t size;
    std::mutex m_Mutex;
    std::vector<WorkPtr> m_Works; //Work是为了防止context开始时因为没有事件注册而直接退出
    std::vector<boost::asio::io_context> m_Services;
    std::vector<std::thread> m_Threads;
    std::size_t m_NextServiceIndex;
};
