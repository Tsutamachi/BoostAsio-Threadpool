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

// contents：指向当前数据块的指针（类型为 void*，需强制转换为 char*）。
// size：每个数据元素的大小（通常为 1 字节，因为 HTTP 传输是字节流）。
// nmemb：数据元素的数量，总字节数为 size * nmemb。
// s：即 &readBuffer，通过 CURLOPT_WRITEDATA 传递的指针。

void HttpMgr::PostHttpReq(const std::string &url, const std::string &json, ReqId req_id, Modules mod)
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    // 初始化函数
    curl = curl_easy_init();
    if (curl) {
        // 定义服务器端地址
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 得到要传递的json对象
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        // 当调用 curl_easy_perform(curl); 执行 HTTP 请求时，
        // libcurl 会与服务器建立连接并接收响应数据。
        // 在接收到数据时，libcurl
        // 会自动调用之前设置的 WriteCallback 函数来处理这些数据
        // 设置写函数和读函数在接受到数据的时候使用curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);函数通过WriteCallback来进行数据的处理
        // CURLOPT_WRITEDATA：
        // 传递一个 void* 类型的指针给 WriteCallback 函数，作为其第四个参数（即代码中的 std::string *s）。
        // 这里传递的是 readBuffer 的地址，用于在回调函数中累积响应数据。

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
// 在执行这一条的时候调用WriteCallback
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            // 如果回调函数被是设置
            if (_callback) {
                // 调用返回给客户端为空,调用回调函数，在客户端中使用注册或者登陆完成的函数
                _callback(req_id, "", ErrorCode::ERR_NETWORK);
            }
        } else {
            if (_callback) {
                // 如果成功那么返回给客户端的就是成功的信息通过回调函数来进行
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
