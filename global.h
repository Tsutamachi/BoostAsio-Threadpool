#ifndef GLOBAL_H
#define GLOBAL_H
#include <functional>
extern std::function<void> repolish;
enum ReqId {
    ID_GET_VARIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002, //注册用户
    ID_LOGIN_USER=1003//登陆用户
};
enum ErrorCode {
    SUCCESS = 0,
    ERR_JSON = 1, //Json解析失败
    ERR_NETWORK = 2,
};
enum Modules {
    REGISTERMOD = 0,
    LOGINMOD = 1,
};
#endif // GLOBAL_H
