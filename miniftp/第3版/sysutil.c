#include"sysutil.h"

int tcp_server(const char *host, unsigned short port)// 创建套接字
{
	// 当port为0时，会随机生成一个端口号
    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)// 如果创建套接字文件错误，用于监听客户连接，listenfd是通信文件描述符
        ERR_EXIT("socket");

    struct sockaddr_in addrSer;// 绑定地址
    addrSer.sin_family = AF_INET;
    addrSer.sin_port = htons(port);
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

int tcp_client()
{
	int sock; // 数据流套接字
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // 
		ERR_EXIT("socket");
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