#ifndef _SESSION_H_//建立会话模块
#define _SESSION_H_

#include"common.h"


typedef struct session//会话结构体
{
		//控制连接
		uid_t uid; // 用户ID
    int ctrl_fd; // 控制连接通道，用于两端的命令通信（即传输命令的通道）
		char cmdline[MAX_COMMOND_LINE_SIZE]; // 定义接收字符串的数组，里面包含命令和参数
		char cmd[MAX_CMD_SIZE]; // 存储命令的数组
		char arg[MAX_ARG_SIZE]; // 存储参数的数组
	
		//数据连接
		struct sockaddr_in *port_addr; // 用于主动连接的地址结构，包括端口和IP地址
		int    data_fd; // 数据连接套接字
		int    pasv_listen_fd; // 用于保存被动连接的监听套接字
		int    data_process;  //用于判断是否处于数据连接状态
	
		//ftp协议状态（控制数据传输格式）
		int is_ascii; // 文件类型是否是ASCII格式
		char *rnfr_name; // 重命名之文件的旧名字
		unsigned long long restart_pos; // 文件续传的位置
		
		//父子进程通道（ftp和nobody进程连接通道）
		int parent_fd; // 父进程套接字
		int child_fd; // 子进程套接字
		
		//限速
		unsigned long long transfer_start_sec; // 传输起始时间（秒）
		unsigned long long transfer_start_usec; // 传输起始时间(微秒，记录微秒是为了更精确)
    
}session_t;

void begin_session(session_t *sess);//开始交谈

#endif /* _SESSION_H_ */
