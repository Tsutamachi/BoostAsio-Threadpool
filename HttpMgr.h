#ifndef HTTPMGR_CPP_H
#define HTTPMGR_CPP_H

#include "defines.h"
#include "Singleton.h"
#include <functional>
#include <map>
#include <memory>
#include <string>

class HttpMgr : public Singleton<HttpMgr>
{
public:
    ~HttpMgr();
    void PostHttpReq(const std::string& url, const std::string& json, ReqId req_id, Modules mod);
    void setCallback(const std::function<void(ReqId, const std::string&, ErrorCode)>& callback);

private:
    friend class Singleton<HttpMgr>;
    HttpMgr();
    std::function<void(ReqId, const std::string&, ErrorCode)> _callback;
};

#endif // HTTPMGR_CPP_H
