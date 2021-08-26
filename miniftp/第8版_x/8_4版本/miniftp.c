#include"sysutil.h"
#include"session.h"
#include"tunable.h"
#include"parseconf.h"
#include"ftpproto.h" // ftp_reply
#include"ftpcodes.h"
#include"hash.h"

void Test_Parseconf()
{
	parseconf_load_file("miniftp.conf"); // 加载配置文件
	printf("tunable_listen_address = %s\n",  tunable_listen_address);
	printf("tunable_pasv_enable = %d\n",  tunable_pasv_enable); 
	printf("tunable_port_enable = %d\n",  tunable_port_enable);
	printf("tunable_listen_port = %d\n",  tunable_listen_port);
	printf("tunable_max_clients = %d\n",  tunable_max_clients);
	printf("tunable_max_per_ip = %d\n",  tunable_max_per_ip);
	printf("tunable_accept_timeout = %d\n",  tunable_accept_timeout);
	printf("tunable_connect_timeout = %d\n",  tunable_connect_timeout);
	printf("tunable_idle_session_timeout = %d\n",  tunable_idle_session_timeout);
	printf("tunable_data_connection_timeout = %d\n",  tunable_data_connection_timeout);
	printf("tunable_local_umask = %d\n",  tunable_local_umask);
	printf("tunable_upload_max_rate = %d\n",  tunable_upload_max_rate);
	printf("tunable_download_mas_rate = %d\n",  tunable_download_max_rate);
}

//全局会话结构指针（在超时断开连接时，会用到这个指针）
session_t *p_sess;

//最大连接数限制相关变量的初始化和函数的声明
static unsigned int s_children_nums = 0; // 子进程的默认个数为0（注意静态变量或函数的名字开头带有s）
static struct hash *s_ip_count_hash; // 定义hash结构体指针类型变量，用于管理用户主机IP与客户连接数的映射的hash桶
static struct hash *s_pid_ip_hash; // 用于管理PID与IP映射的hash桶

static void check_limit(session_t *sess); // 连接限制函数声明
static void handle_sigchld(int sig); // 自定义信号处理方法

unsigned int hash_func(unsigned int buket_size, void *key); // 自定义hash映射函数
unsigned int handle_ip_count(void *ip); // 计算IP与客户连接数的个数
void drop_ip_count(unsigned int *ip);  // 删除PID与IP映射个数

int main(int argc, char *argv[])
{
		//加载配置文件
		parseconf_load_file("miniftp.conf");
		//程序后台化
		//daemon(0, 0); //dev/null：dev=0，表示程序启动后，直接转化到shell的工作目录，第二个0表示不进行重定向。在这里可以将程序运行的信息打印到运行日志中

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
			int    data_process; // 用于判断是否处于数据连接状态
			 
			//ftp协议状态
			int is_ascii;
			char *rnfr_name; // 重命名之文件的旧名字
			unsigned long long restart_pos; // 文件续传的位置
			unsigned int  max_clients; // 客户最大连接总数
			unsigned int  max_per_ip;  // 每IP最大连接数
			
			//父子进程通道
			int parent_fd;
			int child_fd;
			
			//限速
			unsigned long long transfer_start_sec;
			unsigned long long transfer_start_usec;
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
				0, // 默认数据连接处于断开状态
				
				//ftp协议状态格式化
				1, // 文件类型默认是ASCII格式
				NULL, // 默认旧名字为空
				 0, // 续传位置默认为0
				 0,  // 客户最大连接总数初始化为0
				 0,  // 每IP最大连接数初始化为0
				//父子进程通道
				-1, -1,
				
				//限速
				0,0
    };
		p_sess = &sess;
		// printf("ip = %s\n", tunable_listen_address);
		// printf("port = %d\n", tunable_listen_port);
		
		//安装子进程退出信号，处理僵尸进程，并且处理退出的子进程
		signal(SIGCHLD, handle_sigchld);
	
		
		s_ip_count_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func); // 申请用于管理IP与客户连接数的hash表
		s_pid_ip_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func); // 申请用于管理PID与IP映射的hash表
	
		int listenfd = tcp_server(tunable_listen_address,  tunable_listen_port);

    int sockConn; // 通信描述符
    struct sockaddr_in addrCli;
    socklen_t addrlen = sizeof(struct sockaddr); // 接收的地址长度要初始化，否则接收的地址会出现错误;
    while(1)//等待客户端连接
    {
        sockConn = accept(listenfd, (struct sockaddr*)&addrCli, &addrlen);/*被动监听客户端发起的tcp连接请求，三次握手后连接才能建立成功*/
        if(sockConn < 0)//接受连接失败，提醒连接失败
        {
            perror("accept");
            continue;
        }
				//最大连接数控制（感觉这一段可以放在fork后面，这样就无需再s_children_nums自减
				s_children_nums++; // 连接成功，则子进程个数加1
				sess.max_clients = s_children_nums; // 保存
				
				//每IP连接数
				unsigned int ip = addrCli.sin_addr.s_addr; // 获取IP地址
				sess.max_per_ip = handle_ip_count(&ip); // 更新当前IP已经连接的客户个数
				
        // 接收成功，则创建子进程 
        pid_t pid = fork();//创建子进程
        if(pid == -1)//如果创建子进程失败，则直接宕机
				{
					s_children_nums--;
					ERR_EXIT("fork");
				}

        if(pid == 0)//Child Process,里面包含客户端的所有请求
        {
            close(listenfd);// 子进程不用监听客户端的连接请求，所以直接关闭监听套接字
            sess.ctrl_fd = sockConn; // 保存控制连接通道
            
						//连接限制
						check_limit(&sess); // 判断是否超出连接限制，如果超出，则退出当前子进程
						
            begin_session(&sess);//开始会话，创建进程组
            exit(EXIT_SUCCESS);//服务完成，退出会话
        }
        else//Parent Process
        {
        		//增加pid跟ip的映射
						hash_add_entry(s_pid_ip_hash, &pid, sizeof(pid), &ip, sizeof(ip));
	
            close(sockConn);//父进程不用关心客户端连接，所以直接关闭通信描述符
        }
    }
    close(listenfd);
    return 0;
}

static void check_limit(session_t *sess) // 连接限制函数
{
	if(tunable_max_clients!=0 && sess->max_clients>tunable_max_clients) // 如果配置项tunable_max_clients不为0，并且当前的连接数目超过配置项的设定值
	{
		//421 There are too many connected users, please try later.
		ftp_reply(sess, FTP_TOO_MANY_USERS, "There are too many connected users, please try later."); // 发送421代码（连接的客户过多，请稍后重试）
		exit(EXIT_FAILURE); // 失败退出
	}

	if(tunable_max_per_ip!=0 && sess->max_per_ip>tunable_max_per_ip) // 当最大每ip连接数配置项tunable_max_per_ip不为0，并且当前ip的客户连接数目超过配置项的设定值
	{
		// 421 There are too many connections from your internet address.
		ftp_reply(sess, FTP_IP_LIMIT, "There are too many connections from your internet address."); // 发送421代码（提示当前ip已经超出了自身能够连接的最大数）
		exit(EXIT_FAILURE); 
	}
}

static void handle_sigchld(int sig) // 自定义信号处理方法
{
	//减少最大连接数
	s_children_nums--;

	//减少每ip的连接数
	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0) // 循环非阻塞等待子进程退出，如果有子进程退出，我们需要减少PID与IP的映射个数
	{
		unsigned int *ip = (unsigned int *)hash_lookup_entry(s_pid_ip_hash, &pid, sizeof(pid)); // 获取当前PID地址已经连接的客户个数
		if(ip == NULL)
			continue;
		drop_ip_count(ip); // 减少IP对应的客户连接个数
		hash_free_entry(s_pid_ip_hash, &pid, sizeof(pid)); // 释放s_pid_ip_hash表中pid所在的节点
	}
}

unsigned int hash_func(unsigned int buket_size, void *key) // 自定义hash映射函数
{
	return (*(unsigned int*)key) % buket_size; // 除留余数法
}

unsigned int handle_ip_count(void *ip) // 更新当前IP已经连接的客户个数
{
	unsigned int *p_count = (unsigned int *)hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int)); // 获取当前ip地址已经连接的客户个数
	if(p_count == NULL) // 如果保存客户连接数的空间不存在，说明客户端是第一次连接服务器
	{
		unsigned int count = 1;
		hash_add_entry(s_ip_count_hash, ip, sizeof(unsigned int), &count, sizeof(unsigned int));
		return count;
	}

	(*p_count)++; // 当前IP已经连接的客户个数加1
	return *p_count; // 返回已建立连接的客户个数
}

void drop_ip_count(unsigned int *ip) // 减少IP对应的客户连接个数
{
	unsigned int *p_count = (unsigned int*)hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int)); // 获取当前ip地址已经创建的子进程个数
	if(p_count == NULL) // 如果保存子进程个数的空间不存在，说明当前IP是个光杆司令，并没有创建子进程
		return;
		
	(*p_count)--; 
	if(*p_count == 0) // 如果当前IP的子进程个数减少为0，释放s_ip_count_hash表中IP所在的节点
		hash_free_entry(s_ip_count_hash, ip, sizeof(unsigned int));
}