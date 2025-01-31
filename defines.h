#pragma once

//main.cpp
#define PORT 10086

//session--各种包的结构
//包头结构：
// MegId-datalen-sequence-totalpacket
#define HEAD_ID_LEN 2       //该包头 对应数据包的类型
#define HEAD_DATA_LEN 2     //该包头 对应数据包的数据长度
#define HEAD_SEQUENCE_LEN 4 //初包和其他包为0,数据包从1开始
#define HEAD_IDDATA_LEN HEAD_ID_LEN + HEAD_DATA_LEN
#define HEAD_TOTAL_LEN HEAD_ID_LEN + HEAD_DATA_LEN + HEAD_SEQUENCE_LEN //包头总长度

//首包数据包结构：
//文件的id.Client端提出请求后，Server端回应请求并返回一个id.通过SessionUuid+FileId,能准确锁定数据包所属文件。
//非文件数据的包，FILE_ID =0
#define FILE_NAME 256       //文件名的长度--作为首包数据（UTF-8 编码)
#define FILE_SIZE 8         //该文件的总长度
#define TOTAL_PACKETS_NUM 2 //记录这个文件的数据总包数，其他包都为0
#define FILE_HASH 16 //MD5算法。文件Hash值，用来验证文件的完整性--仅在首包中

//普通数据包的结构：
//数据部分加上ID一共为1024
#define FILE_ID 4                    //普通数据包的内容之一
#define FILE_DATA_LEN 1024 - FILE_ID //数据包中数据的长度
//一次发送需要包括包头和数据
#define MAX_LENGTH 1024 + HEAD_TOTAL_LEN //buffer的长度
//一个Client并行上传文件的上限数
#define MAX_UPLOAD_NUM 5

//Lock
#define MAX_SENDQUE 1000 //发送队列的最大长度

//MessageId     前1000一般留给系统级的消息，其余的留给业务级
enum MSG_IDS {
    Test = 1001,
    FileFirstBag = 2001,
    FileDataBag = 2002,
    FileFinalBag = 2003,
    ReturnFileId = 2004
};
