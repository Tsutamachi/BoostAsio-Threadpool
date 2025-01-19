#pragma once

//main
#define PORT 10086

//session--Message Construction
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define HEAD_TOTLE_LEN 4
#define MAX_LENGTH 2048

//Lock
#define MAX_SENDQUE 1000

//MessageId     前1000一般留给系统级的消息，其余的留给业务级
// #define HELLO_WORLD 1001 short类型
enum MSG_IDS { MEG_HELLO_WORLD = 1001 };
