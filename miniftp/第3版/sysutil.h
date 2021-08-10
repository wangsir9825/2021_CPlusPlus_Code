#ifndef _SYSUTIL_H_ // 系统工具模块
#define _SYSUTIL_H_

#include"common.h"

int tcp_server(const char *host, unsigned short port);// 创建套接字
int tcp_client(); // 可以认为是编码角度的客户端

char* statbuf_get_perms(struct stat *sbuf); // 获取文件权限
char* statbuf_get_date(struct stat *sbuf); // 获取文件的时间信息
#endif /* _SYSUTIL_H_ */
