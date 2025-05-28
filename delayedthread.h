#pragma once
#include <iostream>
#include <memory>
#include <thread>

class DelayedThread {
public:
    DelayedThread() = default;

    // 启动线程（仅当未启动时）
    template <typename Callable, typename... Args>
    void start(Callable&& func, Args&&... args) {
        if (!thread_) {
            thread_ = std::make_unique<std::thread>(
                std::forward<Callable>(func),
                std::forward<Args>(args)...
            );
        }
    }

    // 等待线程结束并重置
    void join() {
        if (thread_ && thread_->joinable()) {
            thread_->join();
            thread_.reset();
        }
    }

    // 分离线程并重置
    void detach() {
        if (thread_ && thread_->joinable()) {
            thread_->detach();
            thread_.reset();
        }
    }

    // 析构时自动等待未处理的线程
    ~DelayedThread() {
        if (thread_ && thread_->joinable()) {
            thread_->join();
            thread_.reset();
        }
    }

private:
    std::unique_ptr<std::thread> thread_;
};
