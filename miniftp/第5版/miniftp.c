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
			int    data_fd; // 数据连接套接字
			int    pasv_listen_fd; // 用于保存被动连接的监听套接字
			
			//ftp协议状态
			int is_ascii;
			char *rnfr_name; // 重命名之文件的旧名字
			
			//父子进程通道
			int parent_fd;
			int child_fd;
		}session_t;
	*/
    session_t sess = //会话结构体初始化
    {
				//控制连接初始化
				-1, -1, "", "", "",
				
				//数据连接格式化
				NULL,
				-1, // 默认处于无连接状态
				-1, // 被动连接默认处于断开状态
				
				//ftp协议状态格式化
				1, // 文件类型默认是ASCII格式
				NULL, // 默认旧名字为空
				//父子进程通道
				-1, -1
    };

    int listenfd = tcp_server("172.17.0.4",  9100);// 先将IP写死，后期可以在配置文件中修改，端口选择9100（记住不要选择知名端口，比如20、21、80等，所谓知名端口是指被服务器占用的端口）

    int sockConn; // 通信描述符
    struct sockaddr_in addrCli;
    socklen_t addrlen;
    while(1)//等待客户端连接
    {
        sockConn = accept(listenfd, (struct sockaddr*)&addrCli, &addrlen);/*被动监听客户端发起的tcp连接请求，三次握手后连接才能建立成功*/
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
            close(listenfd);// 子进程不用监听客户端的连接请求，所以直接关闭监听套接字
            sess.ctrl_fd = sockConn;
            begin_session(&sess);//开始会话，创建进程组
            exit(EXIT_SUCCESS);//服务完成，退出会话
        }
        else//Parent Process
        {
            close(sockConn);//父进程不用关心客户端连接，所以直接关闭通信描述符
        }
    }
    close(listenfd);
    return 0;
}
