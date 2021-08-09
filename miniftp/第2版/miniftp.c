#include"sysutil.h"
#include"session.h"

int main(int argc, char *argv[])
{
    if(getuid() != 0)//判断是否为root用户启动
    {
        printf("miniftp : must be started as root.\n");
        exit(EXIT_FAILURE);
    }
	/*
		typedef struct session
		{
			//控制连接
			uid_t uid;
			int ctrl_fd;
			char cmdline[MAX_COMMOND_LINE_SIZE];
			char cmd[MAX_CMD_SIZE];
			char arg[MAX_ARG_SIZE];
			//数据连接
			struct sockaddr_in *port_addr;
			//ftp协议状态
			int is_ascii;
		}session_t;
	*/
    session_t sess = //会话结构体初始化
    {
				//控制连接初始化
				-1, -1, "", "", "",
				
				//数据连接格式化
				NULL,
		
				//ftp协议状态格式化
				1 // 文件类型默认是ASCII格式
    };

    int listenfd = tcp_server("192.168.232.10",  9100);// 先将IP写死，后期可以在配置文件中修改，端口选择9100（记住不要选择知名端口，比如20、21、80等，所谓知名端口是指被服务器占用的端口）

    int sockConn;
    struct sockaddr_in addrCli;
    socklen_t addrlen;
    while(1)//等待客户端连接
    {
        sockConn = accept(listenfd, (struct sockaddr*)&addrCli, &addrlen);//创建监听套接字，用于接收数据
        if(sockConn < 0)//接收失败，提醒连接失败
        {
            perror("accept");
            continue;
        }
        // 接收成功，则创建子进程 
        pid_t pid = fork();//创建子进程
        if(pid == -1)//如果创建子进程失败，则直接宕机
            ERR_EXIT("fork");

        if(pid == 0)//Child Process,里面包含客户端的所有请求
        {
            close(listenfd);// 关闭父进程的套接字
            sess.ctrl_fd = sockConn;
            begin_session(&sess);//开始会话，创建进程组
            exit(EXIT_SUCCESS);//服务完成，退出会话
        }
        else//Parent Process
        {
            close(sockConn);//父进程不用关心客户端连接，所以直接关闭套接字
        }
    }
    close(listenfd);
    return 0;
}
