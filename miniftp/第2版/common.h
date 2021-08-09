/* 公共组件 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h> // 输入输出
#include<unistd.h> // getcwd
#include<stdlib.h>
#include<string.h>


#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<pwd.h> // getpwnam
#include<shadow.h> // 引证头文件
#include<crypt.h> // 加密函数头文件

// 封装错误信息
#define ERR_EXIT(msg)\
    do{\
        perror(msg);\
        exit(EXIT_FAILURE);\
    }while(0)

#define MAX_COMMOND_LINE_SIZE 1024 // 命令行的最大长度
#define MAX_CMD_SIZE          128 // 命令最大长度
#define MAX_ARG_SIZE          1024 // 参数最大长度

#define MAX_BUFFER_SIZE       1024
#define MAX_CWD_SIZE          512 // 当前公共目录

#endif /* _COMMON_H_ */
