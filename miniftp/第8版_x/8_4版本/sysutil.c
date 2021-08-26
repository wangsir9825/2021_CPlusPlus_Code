#include"sysutil.h"

void getlocalip(char *ip) // 获取服务器本地ip
{
	char host[MAX_HOST_NAME_SIZE] = {0};
	if(gethostname(host, MAX_HOST_NAME_SIZE) < 0) // 如果获取主机名失败
		ERR_EXIT("getlocalip");

	struct hostent *pht;
	if((pht = gethostbyname(host)) == NULL) // 如果获取ip地址失败
		ERR_EXIT("gethostbyname");

	strcpy(ip, inet_ntoa(*(struct in_addr*)pht->h_addr)); // 将结构体pht中的ip地址拷贝给ip
}

int tcp_server(const char *host, unsigned short port)// 创建套接字
{
	// 当port为0时，会随机生成一个端口号
    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)// 如果创建套接字文件错误，用于监听客户连接，listenfd是通信文件描述符
        ERR_EXIT("socket");

    struct sockaddr_in addrSer;// 绑定地址
    addrSer.sin_family = AF_INET;
    addrSer.sin_port = htons(port); // 将主机的无符号短整形数转换成网络字节顺序。
    addrSer.sin_addr.s_addr = inet_addr(host);
    
    int on = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)//设置地址重用，防止程序地址被占用 
        ERR_EXIT("setsockopt");
		//绑定监听套接字
    if(bind(listenfd, (struct sockaddr*)&addrSer, sizeof(addrSer)) < 0)//绑定文件描述符，服务器ip和端口号，建立固定连接关系
        ERR_EXIT("bind");

    if(listen(listenfd, SOMAXCONN) < 0)//将服务器端的主动描述符转为被动描述符，用于被动监听客户连接
        ERR_EXIT("listen");

    return listenfd;

}

int tcp_client(int port) // 创建数据流套接字，并绑定端口，这里第port值一般为20,
{
	int sock; // 数据流套接字
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // 0 代表产生随机端口，这个随机端口用于与客户端传输数据
		ERR_EXIT("socket");
		
	struct sockaddr_in address; // IP地址
	address.sin_family = AF_INET; // 设置地址族为IPv4
	address.sin_port = htons(port); // 将主机的无符号短整形数端口号转换成网络字节顺序
	address.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY表示可以选择服务器上的任意一个网卡的IP地址 
	// 为了防止多个客户端同时请求主动连接，需要设置服务器的地址和端口重用
	int on = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");
	// 绑定端口
	if(bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) // 如果绑定20端口出错
		ERR_EXIT("bind 20");
		
	return sock;
}

char* statbuf_get_perms(struct stat *sbuf) // 获取文件权限
{
	static char perms[] = "----------"; // 保存文件权限字符的数组，延长perms数据的生命周期，否则离开这个函数，就失效了
	mode_t mode = sbuf->st_mode;

	switch(mode & S_IFMT) // 获取文件属性
	{
	case S_IFREG: // 普通文件
		perms[0] = '-';
		break;
	case S_IFDIR: // 目录文件
		perms[0] = 'd';
		break;
	case S_IFCHR: // 字符设备
		perms[0] = 'c';
		break;
	case S_IFIFO: // 管道文件
		perms[0] = 'p';
		break;
	case S_IFBLK: // 块设备文件
		perms[0] = 'b';
		break;
	case S_IFLNK: // 链接文件
		perms[0] = 'l';
		break;
	}

// 拥有者用户权限
  if(mode & S_IRUSR)
		perms[1] = 'r';
	if(mode & S_IWUSR)
		perms[2] = 'w';
	if(mode & S_IXUSR)
		perms[3] = 'x';
// 组用户权限
	if(mode & S_IRGRP)
		perms[4] = 'r';
	if(mode & S_IWGRP)
		perms[5] = 'w';
	if(mode & S_IXGRP)
		perms[6] = 'x';
// 其它用户权限
	if(mode & S_IROTH)
		perms[7] = 'r';
	if(mode & S_IWOTH)
		perms[8] = 'w';
	if(mode & S_IXOTH)
		perms[9] = 'x';
    
	return perms;
}

char* statbuf_get_date(struct stat *sbuf) // 获取文件的时间信息
{
	static char date[64] = {0};
	struct tm *ptm = localtime(&sbuf->st_mtime); // 获取时间信息
	strftime(date, 64, "%b %e %H:%M", ptm); // 将时间信息进行格式化，b：月份的简写，e：天，H：24小时制，M：十进制分钟
	return date;
}

void send_fd(int sock_fd, int fd) // 发送套接字
{
	int ret;
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(fd))];
	int *p_fds;
	char sendchar = 0;
	 // 将所有消息结构填充在msg里面
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	p_fds = (int*)CMSG_DATA(p_cmsg);
	*p_fds = fd; // 将fd套接字填充到msg

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);
	ret = sendmsg(sock_fd, &msg, 0); // 利用sock_fd通道，发送msg
	if (ret != 1)
		ERR_EXIT("sendmsg");
}

int recv_fd(const int sock_fd) // 接收套接字
{
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr *p_cmsg;
	int *p_fd;

	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;  
	ret = recvmsg(sock_fd, &msg, 0);	// 将接收到的所有消息结构填充在msg里面
	if (ret != 1)
		ERR_EXIT("recvmsg");

	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
		ERR_EXIT("no passed fd");


	p_fd = (int*)CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if (recv_fd == -1)
		ERR_EXIT("no passed fd");

	return recv_fd;
}

static struct timeval s_cur_time;
unsigned long long get_time_sec()
{
	if(gettimeofday(&s_cur_time, NULL) < 0) // 如果获取当前时间失败（NULL代表当前时区）
		ERR_EXIT("gettimeofday");
	return s_cur_time.tv_sec; // 返回当前的秒
}
unsigned long long get_time_usec()
{
	return s_cur_time.tv_usec; // 返回当前的微秒
}

void nano_sleep(double sleep_time) // 休眠函数3.28
{
	unsigned long sec = (unsigned long)sleep_time; // 计算时间秒的整数部分，假设时间是3.28，则sec=3
	double decimal = sleep_time - (double)sec; // 计算时间秒的小数部分，假设时间是3.28，则decimal=0.28
	// 构造nanosleep函数的参数ts
	struct timespec ts;
	ts.tv_sec = (time_t)sec; // 填充秒
	ts.tv_nsec = (long)(decimal * 1000000000); // 填充纳秒（1秒等于10亿纳秒）

	int ret;
	do // 为了防止休眠期间被信号打断，我们用一个循环解决这个问题
	{
		ret = nanosleep(&ts, &ts);
	}
	while (ret==-1 && errno==EINTR); //当返回值为-1并且是信号中断，则继续休眠（相当于忽略了信号中断。nanosleep正常执行结束后，返回值不是-）
}