#pragma once
//Server、Client的监听端口
#define PORT 10086
#define DataPlace "/root/DataPlace/"

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

//Lock
#define MAX_SENDQUE 100000 //发送队列的最大长度

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
};
