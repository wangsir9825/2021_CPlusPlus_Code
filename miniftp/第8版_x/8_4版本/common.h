/* 公共组件 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h> // 输入输出
#include<unistd.h> // getcwd、daemon、gethostname
#include<stdlib.h>
#include<string.h>
#include<assert.h> // assert

#include<sys/socket.h> // accept
#include<netinet/in.h>
#include<arpa/inet.h> // inet_ntoa
#include<netdb.h> // gethostbyname

#include<pwd.h> // getpwnam
#include<shadow.h> // 引证头文件
#include<crypt.h> // 加密函数头文件

#include<dirent.h> // opendir
#include<sys/stat.h> // stat

#include<time.h> // 时间结构
#include<sys/time.h> // gettimeofday

#include<errno.h> // errno
#include<fcntl.h> // open read 

#include<signal.h> // signal

#include<sys/wait.h> // waitpid

#include<linux/capability.h> // capset
#include<sys/syscall.h> // syscall

// 封装错误信息
#define ERR_EXIT(msg)\
    do{\
        perror(msg);\
        exit(EXIT_FAILURE);\
    }while(0)

#define MAX_COMMOND_LINE_SIZE 1024 // 命令行的最大长度
#define MAX_CMD_SIZE          128 // 命令最大长度
#define MAX_ARG_SIZE          1024 // 参数最大长度

#define MAX_BUFFER_SIZE       1024 // 缓冲区最大值
#define MAX_CWD_SIZE          512 // 当前公共目录

#define MAX_SETTING_LINE_SIZE 1024 // 配置行最大长度
#define MAX_KEY_SIZE          128 // 每个配置项的关键字最大长度
#define MAX_VALUE_SIZE        512 // 每个配置项的值的最大长度

#define MAX_BUCKET_SIZE       191 // 哈希桶的大小
#define MAX_HOST_NAME_SIZE    256 // 主机名
#endif /* _COMMON_H_ */
