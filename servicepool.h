#pragma once
#include "Singleton.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using WorkPtr = std::unique_ptr<boost::asio::io_context::work>;

class ServicePool : public Singleton<ServicePool>
{
    friend class Singleton<ServicePool>;

public:
    ServicePool(const ServicePool &) = delete;
    ServicePool &operator=(const ServicePool &) = delete;

    boost::asio::io_context &GetService();

    void Stop();
    ~ServicePool();

private:
    ServicePool(std::size_t size = std::thread::hardware_concurrency());

    std::mutex m_Mutex;
    std::vector<WorkPtr> m_Works; //Work是为了防止context开始时因为没有事件注册而直接退出
    std::vector<boost::asio::io_context> m_Services;
    std::vector<std::thread> m_Threads;
    std::size_t m_NextServiceIndex;
};
