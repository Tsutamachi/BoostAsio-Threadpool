#include "HttpMgr.h"
#include <curl/curl.h>
#include <iostream>

// 回调函数，用于处理curl响应
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try {
        s->append((char *) contents, newLength);
    } catch (std::bad_alloc &e) {
        return 0;
    }
    return newLength;
}

HttpMgr::~HttpMgr()
{
    curl_global_cleanup();
}

HttpMgr::HttpMgr()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void HttpMgr::PostHttpReq(const std::string &url, const std::string &json, ReqId req_id, Modules mod)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        // 当调用 curl_easy_perform(curl); 执行 HTTP 请求时，
        // libcurl 会与服务器建立连接并接收响应数据。
        // 在接收到数据时，libcurl
        // 会自动调用之前设置的 WriteCallback 函数来处理这些数据
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
// 在执行这一条的时候调用WriteCallback
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            if (_callback) {
                _callback(req_id, "", ErrorCode::ERR_NETWORK);
            }
        } else {
            if (_callback) {
                _callback(req_id, readBuffer, ErrorCode::SUCCESS);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void HttpMgr::setCallback(const std::function<void(ReqId, const std::string &, ErrorCode)> &callback)
{
    _callback = callback;
}
