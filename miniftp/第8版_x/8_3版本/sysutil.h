#ifndef _SYSUTIL_H_ // 系统工具模块
#define _SYSUTIL_H_

#include"common.h"

void getlocalip(char *ip); // 获取服务器本地ip
int tcp_server(const char *host, unsigned short port);// 创建套接字
int tcp_client(int port); // 创建数据流套接字，并绑定端口

char* statbuf_get_perms(struct stat *sbuf); // 获取文件权限
char* statbuf_get_date(struct stat *sbuf); // 获取文件的时间信息

void send_fd(int sock_fd, int fd); // 发送套接字
int  recv_fd(const int sock_fd); // 接收套接字

unsigned long long get_time_sec(); // 获取当前时间的秒
unsigned long long get_time_usec(); // 获取当前时间的微秒
void nano_sleep(double sleep_time); //休眠函数（这个函数是对nanosleep的封装）

#endif /* _SYSUTIL_H_ */
