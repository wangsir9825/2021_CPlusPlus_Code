#include"privparent.h"//nobody 服务进程
#include"session.h"
#include"sysutil.h"
#include"privsock.h"

//获取主动连接套接字
static void privop_pasv_get_data_sock(session_t *sess);
//判定被动模式是否被激活
static void privop_pasv_active(session_t *sess);
//创建被动模式的监听套接字
static void privop_pasv_listen(session_t *sess);
//被动模式下接收连接
static void privop_pasv_accept(session_t *sess);

void handle_parent(session_t *sess)
{
		// 更改nobody进程名
		struct passwd *pwd = getpwnam("nobody"); //获取nobody相关信息
		if(pwd == NULL)
			ERR_EXIT("getpwnam");
		if(setegid(pwd->pw_gid) < 0) // 将root子进程的群ID更改为nobody的群ID如果失败则报错
			ERR_EXIT("setegid");
		if(seteuid(pwd->pw_uid) < 0) // 将root子进程的ID更改为nobody的ID如果失败则报错
			ERR_EXIT("seteuid");	
		char cmd;
    while(1)//不停的等待ftp服务进程的消息
    {
				//不停的等待ftp服务进程的消息
				cmd = priv_sock_get_cmd(sess->parent_fd); // 阻塞等待接收命令
				switch(cmd)
				{
				case PRIV_SOCK_GET_DATA_SOCK: // 请求获取主动连接套接字命令
					privop_pasv_get_data_sock(sess); // 获取主动连接套接字
					break;
				case PRIV_SOCK_PASV_ACTIVE: // 请求获取被动模式是否被激活命令
					privop_pasv_active(sess); // 判定被动模式是否被激活
					break;
				case PRIV_SOCK_PASV_LISTEN: // 请求创建被动模式的监听套接字命令
					privop_pasv_listen(sess); //创建被动模式的监听套接字
					break;
				case PRIV_SOCK_PASV_ACCEPT: // 请求接受客户端连接命令
					privop_pasv_accept(sess);//被动模式下接收连接
					break;
				}      
            
    }

}

static void privop_pasv_get_data_sock(session_t *sess)//获取主动连接套接字
{
	//接收ftp进程发送的ip
	char ip[16] = {0};
	int len = priv_sock_get_int(sess->parent_fd);
	priv_sock_recv_buf(sess->parent_fd, ip, len);

	//接收ftp进程发送的port
	unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);

	struct sockaddr_in addr; 
	// 将IP和端口号填充在addr里面
	addr.sin_family = AF_INET; // 设置地址族为IPv4
	addr.sin_port = htons(port); // 将主机的无符号短整形数转换成网络字节顺序。
	addr.sin_addr.s_addr = inet_addr(ip); // 将IP字符串转换为IP地址

	int sock = tcp_client(); // 创建数据流套接字
	socklen_t addrlen = sizeof(struct sockaddr); // 地址长度
	if(connect(sock, (struct sockaddr*)&addr, addrlen) < 0) // 如果服务器主动连接客户端失败
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD); //nobody 进程回复连接失败
		return;
	}

	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK); //nobody 进程回复连接成功
	priv_sock_send_fd(sess->parent_fd, sock); // 将被激活的套接字发送给ftp进程

	close(sock); // nobody已经不需要数据连接套接字，所以要关闭
}

static void privop_pasv_active(session_t *sess)//判定被动模式是否被激活
{
	int active = -1; //未激活
	if(sess->pasv_listen_fd != -1) // 如果被动模式被激活
		active = 1; //激活
	priv_sock_send_int(sess->parent_fd, active); 
}

static void privop_pasv_listen(session_t *sess)//创建被动模式的监听套接字
{
	char ip[16] = "172.17.0.4"; // 服务器的IP
	unsigned int v[4] = {0};
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]); // 将IP分割


	int sockfd = tcp_server(ip, 0);	//服务器处于监听状态，0代表生成的默认端口号，sockfd是监听套接字

	struct sockaddr_in addr; // 地址结构体
	socklen_t addrlen = sizeof(struct sockaddr); // 地址长度
	if(getsockname(sockfd,	(struct sockaddr*)&addr, &addrlen) < 0) // 如果获取套接字的名字失败
		ERR_EXIT("getsockname");

	unsigned short port = ntohs(addr.sin_port); // 获取端口号，sin_port是网络字节序，ntohs可以将sin_port转化为本地字节序
	sess->pasv_listen_fd = sockfd; // 将被动连接套接字保存在会话结构体中
	

	//将服务器的ip发送给ftp进程
	priv_sock_send_int(sess->parent_fd, strlen(ip)); // 通过parent_fd通道发送字符串的长度给ftp进程
	priv_sock_send_buf(sess->parent_fd, ip, strlen(ip)); // 通过parent_fd通道发送IP字符串给ftp进程
	//将服务器的port发送给ftp进程
	priv_sock_send_int(sess->parent_fd, (int)port); // 通过parent_fd通道发送端口号给ftp进程
}

static void privop_pasv_accept(session_t *sess)//被动模式下接收连接
{
	int sockConn; // 
	struct sockaddr_in addr; // 地址结构
	socklen_t addrlen; // 地址长度
	if((sockConn = accept(sess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen)) < 0)// 服务器接受客户端的连接，并产成一个用于数据传输的套接字sockConn
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD); //发送连接失败应答
		return;
	}
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK); //发送连接成功应答

	priv_sock_send_fd(sess->parent_fd, sockConn); // 发送套接字
	
	close(sess->pasv_listen_fd);
	sess->pasv_listen_fd = -1;
	close(sockConn);
}
