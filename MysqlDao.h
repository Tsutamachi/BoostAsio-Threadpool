#pragma once
//数据库池
#include "defines.h"
#include <thread>
#include <memory>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "mysql.h"


class SqlConnection {
public:
    SqlConnection(MYSQL* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
    std::unique_ptr<MYSQL> _con;
    int64_t _last_oper_time;
};

class MySqlPool {
public:
    MySqlPool(const std::string& host,const std::string& port, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : host_(host) ,port_(port), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false){
        try {
            for (int i = 0; i < poolSize_; ++i) {
                MYSQL* con = mysql_init(NULL);

                if (mysql_real_connect(con, host.c_str(), user.c_str(), pass.c_str(), schema.c_str(), std::stoi(port), NULL, 0) == NULL) {
                    fprintf(stderr, "mysql_real_connect() failed\n");
                }
                if (con) {
                    printf("数据库已经启动");
                    printf("Connected to the database successfully!\n");
                } else {
                    fprintf(stderr, "Connection failed\n");
                }
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));
            }

            _check_thread = std::thread([this]() {
                while (!b_stop_) {
                    checkConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
            });

            _check_thread.detach();
        }
        catch (std::exception& e) {
            std::cout << "mysql pool init failed, error is " << e.what()<< std::endl;
        }
    }

    void checkConnection() {
        std::cout<<"检测连接";
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer([this, &con]() {
                pool_.push(std::move(con));
            });

            if (timestamp - con->_last_oper_time < 5) {
                continue;
            }

            // 使用MariaDB C API检查连接并在必要时重新连接
            MYSQL* mysql = con->_con.get(); // 获取原始指针
            if (mysql_ping(mysql) != 0) {
                // 释放旧连接
                con->_con.reset();

                // 创建并连接新连接
                MYSQL* new_conn = mysql_init(nullptr);
                if (new_conn == nullptr ||
                    mysql_real_connect(new_conn,host_.c_str(), user_.c_str(), pass_.c_str(), schema_.c_str(), std::stoi(port_), NULL, 0) == nullptr) {
                    std::cerr << "Reconnect failed\n";
                    fprintf(stderr, "  Error message: %s\n", mysql_error(mysql));
                    if (new_conn) mysql_close(new_conn);
                    return;
                }

                // 将新连接交给智能指针管理
                con->_con.reset(new_conn);
                con->_last_oper_time = timestamp;
            }



        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty(); });
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

    ~MySqlPool() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string host_;
    std::string port_;
    std::string user_;
    std::string pass_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread;
};

struct UserInfo {
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd,const std::string& server);
    // int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
    // bool CheckEmail(const std::string& name, const std::string & email);
    // bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    // bool TestProcedure(const std::string& email, int& uid, string& name);
private:
    std::unique_ptr<MySqlPool> pool_;
    bool _isConnected;  // 记录连接状态
};


