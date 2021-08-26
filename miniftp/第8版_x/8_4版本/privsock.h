#ifndef _PRIV_SOCK_H_ // 私有通信模块，用于ftp进程和nobody进程进行通信
#define _PRIV_SOCK_H_

#include"common.h"
#include"session.h"

//FTP服务进程向nobody进程请求的命令
#define PRIV_SOCK_GET_DATA_SOCK 1 // 请求获取主动连接套接字
#define PRIV_SOCK_PASV_ACTIVE 2 // 获取被动模式是否被激活
#define PRIV_SOCK_PASV_LISTEN 3 // 被动模式的监听套接字
#define PRIV_SOCK_PASV_ACCEPT 4 // 请求接受连接

//nobody 进程对FTP服务进程的应答
#define PRIV_SOCK_RESULT_OK 1 // 应答成功
#define PRIV_SOCK_RESULT_BAD 2 // 应答失败

// 内部通信方法
void priv_sock_init(session_t *sess); // 初始化内部进程间通信通道 
void priv_sock_close(session_t *sess); // 关闭内部进程间通信通道
void priv_sock_set_parent_context(session_t *sess); // 设置父进程环境
void priv_sock_set_child_context(session_t *sess); // 设置子进程环境
void priv_sock_send_cmd(int fd, char cmd); // 发送命令(子—>父)
char priv_sock_get_cmd(int fd); // 接收命令(父<-- 子) 
void priv_sock_send_result(int fd, char res); // 发送结果(父—>子)
char priv_sock_get_result(int fd); // 接收结果(子<--父) 
void priv_sock_send_int(int fd, int the_int); // 发送一个整数
int priv_sock_get_int(int fd); // 接收一个整数
void priv_sock_send_buf(int fd, const char *buf, unsigned int len); // 发送一个字符串
void priv_sock_recv_buf(int fd, char *buf, unsigned int len); // 接收一个字符串
void priv_sock_send_fd(int sock_fd, int fd); // 发送文件描述符
int priv_sock_recv_fd(int sock_fd); // 接收文件描述符


#endif /* _PRIV_SOCK_H_ */