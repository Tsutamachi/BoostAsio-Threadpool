#pragma once
#include "defines.h"
#include "hiredis/hiredis.h"
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"
#include <condition_variable>
#include <thread>
#include <memory>
class RedisConPool {
public:
    // redis?
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd), counter_(0){
        for (size_t i = 0; i < poolSize_; ++i) {
            // ?????poolsize?
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }
            // ????????
            // ???????????
            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "error" << std::endl;
                freeReplyObject(reply);
                redisFree(context);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "success" << std::endl;
            connections_.push(context);
        }

        check_thread_ = std::thread([this]() {
            while (!b_stop_) {
                counter_++;
                if (counter_ >= 60) {
                    checkThread();
                    counter_ = 0;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1)); //
            }
        });

    }

    ~RedisConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    void ClearConnections() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            // 判断是否线程池里还有可以用的如果没有就阻塞等待
            return !connections_.empty();
            });
        if (b_stop_) {
            return  nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
        check_thread_.join();
    }

private:
    void checkThread() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        auto pool_size = connections_.size();
        for (int i = 0; i < pool_size && !b_stop_; i++) {
            auto* context = connections_.front();
            connections_.pop();
            try {
                auto reply = (redisReply*)redisCommand(context, "PING");
                if (!reply) {
                    std::cout << "redis ping failed" << std::endl;
                    connections_.push(context);
                    continue;
                }
                freeReplyObject(reply);
                connections_.push(context);
            }
            catch(std::exception& exp){
                std::cout << "Error keeping connection alive: " << exp.what() << std::endl;
                redisFree(context);
                context = redisConnect(host_, port_);
                if (context == nullptr || context->err != 0) {
                    if (context != nullptr) {
                        redisFree(context);
                    }
                    continue;
                }

                auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
                if (reply->type == REDIS_REPLY_ERROR) {
                    std::cout << "error" << std::endl;
                    freeReplyObject(reply);
                    continue;
                }
                freeReplyObject(reply);
                std::cout << "success" << std::endl;
                connections_.push(context);
            }
        }
    }
    std::atomic<bool> b_stop_;
    // ????
    size_t poolSize_;
    // Redis ????????
    const char* host_;
    const char* pwd_;
    int port_;
    // ?????????? Redis ???
    std::queue<redisContext*> connections_;
    // ???????????????????
    std::mutex mutex_;
    // ?????????????
    std::condition_variable cond_;
    // ??????????????
    std::thread  check_thread_;
    // ??????????????????
    int counter_;
};

class RedisMgr: public Singleton<RedisMgr>,
    public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string &key, std::string& value);
    bool Set(const std::string &key, const std::string &value);
    bool LPush(const std::string &key, const std::string &value);
    bool LPop(const std::string &key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string &key, const std::string  &hkey, const std::string &value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string &key, const std::string &hkey);
    bool HDel(const std::string& key, const std::string& field);
    bool Del(const std::string &key);
    bool ExistsKey(const std::string &key);
    void Close() {
        _con_pool->Close();
        _con_pool->ClearConnections();
    }
private:
    RedisMgr();
    std::unique_ptr<RedisConPool>  _con_pool;
};

