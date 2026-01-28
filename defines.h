#pragma once
//mydefins.h
//Server、Client的监听端口
#define SERVERPORT 8080
#define CLIENTPORT 721
#define LOCALHOST_PORT 80
#define DataPlace "/root/DataPlace/"
#define StorageHardDisk "/" //装数据所在盘

//包头结构：
// MegId-Datalen
#define HEAD_ID_LEN 2       //该包头 对应数据包的类型
#define HEAD_DATA_LEN 2     //该包头 对应数据包的数据长度
#define HEAD_TOTAL_LEN HEAD_ID_LEN + HEAD_DATA_LEN //包头总长度

//首包数据包结构：
//文件的id.Client端提出请求后，Server端回应请求并返回一个id.通过SessionUuid+FileId,能准确锁定数据包所属文件。
//非文件数据的包，FILE_ID =0
#define FILE_NAME 256       //文件名的长度--作为首包数据（UTF-8 编码)
#define FILE_SIZE 8         //该文件的总长度
#define TOTAL_PACKETS_NUM 4 //记录这个文件的数据总包数，其他包都为0
#define FILE_HASH 16 //MD5算法。文件Hash值，用来验证文件的完整性--仅在首包中

//普通数据包的结构：
//数据部分加上ID一共为1024
#define MAX_LENGTH 2048              //CSession中m_Data的长度
#define Hash_Verify_Block FILE_DATA_LEN * 10 //一次hash验证10包
#define FILE_ID 1                    //普通数据包的内容之一
#define FILE_SEQUENCE_LEN 4          //从0开始。更好计算此包在文件中是首地址
// #define FILE_DATA_LEN (1024 - FILE_ID - FILE_SEQUENCE_LEN) //数据包中数据的长度
#define FILE_DATA_LEN 1024 //数据包中数据的长度

//一个Client并行上传文件的上限数
#define MAX_UPLOAD_NUM 5
#define MAX_DOWNLOAD_NUM 5
#define CheckLostPerBags 100//上传or下载时，每XX次检查一次是否缺包

//Lock
#define MAX_SENDQUE 500000 //发送队列的最大长度

//MessageId     前1000一般留给系统级的消息，其余的留给业务级
enum MSG_IDS {
    Test = 1001,
    Echo = 1002,
    Back = 1003,
    FileUploadRequest = 2001, //Client应用层->Client处理层--发起传输文件的请求
    FileDownloadRequest = 2002, //Client、Server（接受文件端）应用层->处理层--发起下载文件的请求
    RequestFileId = 2003,     //Client->Server--希望获取一个FileId
    ReturnFileId = 2004,      //Server->Client--接受传输文件的请求
    RejectUpload = 2005,      //Server->Client--没有FileId能够分配，拒绝上传
    StartUpload = 2006,       //Server->Client--请求Client开始发送数据
    FileDataBag = 2007,       //Client->Server--发送文件的   数据包
    FileFinalBag = 2008,      //Client->Server--发送文件的最后数据包
    FileFinish = 2009,        //Client->Server--发送文件完毕
    TellLostBag = 2010,       //Server->Client--告知丢包
    FileLostBag = 2011,       //Client->Server--发送丢失的包
    ServerRecevFinal = 2013,  //Server->Client--告知对端收到FinalBag
    FileComplete = 2012,      //Server->Client--告知文件完整
    RequestVerify = 2014,     //Server->Client--请求对方分块发送hash验证码
    VerifyCode = 2015,        //Client->Server--发送的hash验证码
    SendDamagedBlock = 2016,  //Server->Client--发送有问题的hash-seq
    ReTranDamagedHash = 2017, //Client->Server--重传的数据
    ReTransLostBagFinished = 2018,   //Client->Server--重传缺包数据完成

    Http_GET = 3001,
    Http_POST = 3002,

    MSGID_MAX = Http_POST,//记录MsgId中的最大值，来做有效性判定
    MSGID_MIN = Test
};


//const.h//QML中创建Client对象
#include <functional>
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


//global.h
#include <functional>
extern std::function<void> repolish;
enum ReqId {
    ID_GET_VARIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002,        //注册用户
};
enum ErrorCode {
    SUCCESS = 0,
    ERR_JSON = 1, //Json解析失败
    ERR_NETWORK = 2,
};
enum Modules {
    REGISTERMOD = 0,
};
