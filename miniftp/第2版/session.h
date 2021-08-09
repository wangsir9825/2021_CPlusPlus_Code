#ifndef _SESSION_H_//建立会话模块
#define _SESSION_H_

#include"common.h"


typedef struct session//会话结构体
{
		//控制连接
		uid_t uid; // 用户ID
    int ctrl_fd;
		char cmdline[MAX_COMMOND_LINE_SIZE]; // 定义接收字符串的数组，里面包含命令和参数
		char cmd[MAX_CMD_SIZE]; // 存储命令的数组
		char arg[MAX_ARG_SIZE]; // 存储参数的数组
	
		//数据连接
		struct sockaddr_in *port_addr; // 端口和IP地址
	
		//ftp协议状态
		int is_ascii; // 文件类型是否是ASCII格式
    
}session_t;

void begin_session(session_t *sess);//开始交谈

#endif /* _SESSION_H_ */
