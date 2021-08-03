/* 公共组件 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h> // 输入输出
#include<unistd.h> // 
#include<stdlib.h>
#include<string.h>


#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

// 封装错误信息
#define ERR_EXIT(msg)\
    do{\
        perror(msg);\
        exit(EXIT_FAILURE);\
    }while(0)


#endif /* _COMMON_H_ */
