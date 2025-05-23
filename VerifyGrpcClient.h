#pragma once
#include "defines.h"
#include "message.grpc.pb.h"
#include "Singleton.h"
#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <queue>
#include <condition_variable>
using grpc::Channel;
// 上下文
using grpc::ClientContext;
using grpc::Status;
// 请求
using message::GetVarifyReq;
// 回包
using message::GetVarifyRsp;
// 服务
using message::VarifyService;
// 写一个池
class RPConPool {
public:
    RPConPool(size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
        for (size_t i = 0; i < poolSize_; ++i) {
            // 为每一个连接创建与服务器通信的信使
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port,
                grpc::InsecureChannelCredentials());
            connections_.push(VarifyService::NewStub(channel));
        }
    }

    ~RPConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        // 告诉挂起的线程要关闭池子了
        Close();
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    std::unique_ptr<VarifyService::Stub> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            // 如果此时队列为空则挂起直至有人通知他
            return !connections_.empty();
            });
        //如果停止则直接返回空指针
        if (b_stop_) {
            return  nullptr;
        }
        // 说明队列不为空移动出来复制
        auto context = std::move(connections_.front());
        connections_.pop();
        return context;
    }
// 连接用完了放回还要给其他人用用完了需要回收
    void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connections_.push(std::move(context));
        // 只用还一个连接
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VarifyService::Stub>> connections_;
    // 互斥量
    std::mutex mutex_;
    std::condition_variable cond_;
};

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;

public:
    ~VerifyGrpcClient() {}
    GetVarifyRsp GetVarifyCode(std::string email)
    {
        ClientContext context;
        GetVarifyRsp reply;
        GetVarifyReq request;
        // 将email设置到请求中
        request.set_email(email);
        auto stub=pool_->getConnection();
        // 设置请求后返回一个状态
        Status status = stub->GetVarifyCode(&context, request, &reply);

        if (status.ok()) {
            pool_->returnConnection(std::move(stub));
            return reply;
        } else {
            pool_->returnConnection(std::move(stub));
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
    }

private:
    VerifyGrpcClient();

    std::unique_ptr<RPConPool> pool_;
};
