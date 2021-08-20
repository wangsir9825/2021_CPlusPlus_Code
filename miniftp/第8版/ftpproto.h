// ftp服务进程
#ifndef _FTPPROTO_H_
#define _FTPPROTO_H_

#include"common.h"
#include"session.h"
//不停的等待客户端的命令并做出处理
void handle_child(session_t *sess);//子进程的方法，将会话结构体指针传递给该方法
void ftp_reply(session_t *sess, unsigned int code, const char *text);

#endif /* _FTPPROTO_H_ */
