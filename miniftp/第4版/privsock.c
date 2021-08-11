#include"privsock.h" // 通信模块的实现
#include"sysutil.h"

void priv_sock_init(session_t *sess) // 初始化内部进程间通信通道 
{
	int sockfds[2];
	if(socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0) // 创建全双工sockfds,相当于全双工管道，结构是PF_UNIX，SOCK_STREAM代表流式套接字
		ERR_EXIT("socketpair");
	sess->parent_fd = sockfds[0]; // 具备读写权限的通道
	sess->child_fd = sockfds[1];
}

void priv_sock_close(session_t *sess) // 关闭内部进程间通信通道
{
	if(sess->parent_fd != -1)
	{
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}
	if(sess->child_fd != -1)
	{
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}

void priv_sock_set_parent_context(session_t *sess) // 设置父进程上下文环境
{
	if(sess->child_fd != -1) // 关闭子进程的通信通道
	{
		close(sess->child_fd );
		sess->child_fd = -1;
	}
}

void priv_sock_set_child_context(session_t *sess) // 设置子进程上下文环境
{
	if(sess->parent_fd != -1)
	{
		close(sess->parent_fd); // 关闭父进程的通信通道
		sess->parent_fd = -1;
	}
}

void priv_sock_send_cmd(int fd, char cmd) // 发送命令(子—>父)
{
	int ret = send(fd, &cmd, sizeof(cmd), 0); // ftp进程通过fd将cmd命令发送给nobody进程，0代表发送模式默认的方式。ret为命令的长度
	if(ret != sizeof(cmd))
		ERR_EXIT("priv_sock_send_cmd");
}

char priv_sock_get_cmd(int fd) // 接收命令(父<-- 子) 
{
	printf("wwwwwwwwwwwww\n");
	char cmd;
	int ret = recv(fd, &cmd, sizeof(cmd), 0); // 阻塞等待接收命令，从fd通道接收cmd命令，接收方式为默认，ret为ftp进程接收到命令的长度
	if(ret == 0) // ret为0说明ftp进程没有接收到命令
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(cmd)) // ret不等于命令的长度，说明ftp接收命令错误
	{
		printf("aaaaaaaaaaaa\n");
		ERR_EXIT("priv_sock_get_cmd");
	}
	return cmd; // 成功则将命令返回
}

void priv_sock_send_result(int fd, char res) // 发送结果(父—>子)
{
	int ret = send(fd, &res, sizeof(res), 0);
	if(ret != sizeof(res))
		ERR_EXIT("priv_sock_send_result");
}

char priv_sock_get_result(int fd) // 接收结果(子<--父) 
{
	char res;
	int ret = recv(fd, &res, sizeof(res), 0);
	if(ret == 0)
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(res))
		ERR_EXIT("priv_sock_get_result");
	return res;
}

void priv_sock_send_int(int fd, int the_int) // 发送一个整数
{
	int ret = send(fd, &the_int, sizeof(the_int), 0);
	if(ret != sizeof(the_int))
		ERR_EXIT("priv_sock_send_int");
}

int priv_sock_get_int(int fd)// 接收一个整数
{
	int the_int;
	int ret = recv(fd, &the_int, sizeof(the_int), 0);
	if(ret == 0)
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(the_int))
		ERR_EXIT("priv_sock_get_int");
	return the_int;
}

void priv_sock_send_buf(int fd, const char *buf, unsigned int len) // 发送一个字符串
{
	priv_sock_send_int(fd, len); // 字节流的发送方式没办法判断字符串的长度，所以先告诉对方字符串的长度
	int ret = send(fd, buf, len, 0);
	if(ret != len)
		ERR_EXIT("priv_sock_send_buf");
}

void priv_sock_recv_buf(int fd, char *buf, unsigned int len) // 接收一个字符串，这个len是代表buf的长度
{
	int recv_len = priv_sock_get_int(fd); // 接收字符串的长度
	if(recv_len > len) // 如果接收到的长度比缓冲区的长度还要到，说明数据有溢出
		ERR_EXIT("priv_sock_recv_buf");

	int ret = recv(fd, buf, len, 0); // 从通道fd接收的数据保存在buf中
	if(ret != recv_len) // 如果真实接收到的数据长度ret不等于对方发送的数据长度
		ERR_EXIT("priv_sock_recv_buf");
}

void priv_sock_send_fd(int sock_fd, int fd) // 发送文件描述符
{
	send_fd(sock_fd, fd); // 通过sock_fd通道发送fd套接字，需要把其上下文环境也发送过去
}

int priv_sock_recv_fd(int sock_fd) // 接收文件描述符
{
	return recv_fd(sock_fd); // 通过sock_fd通道接收fd套接字
}
