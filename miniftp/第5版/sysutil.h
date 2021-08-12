#ifndef _SYSUTIL_H_ // 系统工具模块
#define _SYSUTIL_H_

#include"common.h"

int tcp_server(const char *host, unsigned short port);// 创建套接字
int tcp_client(); // 可以认为是编码角度的客户端

char* statbuf_get_perms(struct stat *sbuf); // 获取文件权限
char* statbuf_get_date(struct stat *sbuf); // 获取文件的时间信息

void send_fd(int sock_fd, int fd); // 发送套接字
int  recv_fd(const int sock_fd); // 接收套接字
#endif /* _SYSUTIL_H_ */
