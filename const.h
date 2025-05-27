#pragma once
#include "ConfigMgr.h"
#include "singleton.h"
#include <assert.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,//json验证失败
    RPCFailed = 1002,//邮箱验证启动
    VarifyExpired = 1003,//验证码过期
    VarifyCodeErr = 1004,//验证码不匹配
    UserExist = 1005,//用户名已存在！
    PasswdErr = 1006,//密码不匹配
    // EmailNotMatch = 1007,
    // PasswdUpFailed = 1008,
    PasswdInvalid = 1009,//用户名不存在或密码错误
};
class Defer
{
public:
    Defer(std::function<void()> func)
        : func_(func)
    {}

    ~Defer() { func_(); }

private:
    std::function<void()> func_;
};

#define CODEPREFIX "code_"
