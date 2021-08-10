#include"ftpproto.h"
#include"session.h"
#include"str.h"
#include"ftpcodes.h"
#include"sysutil.h"

static void ftp_reply(session_t *sess, unsigned int code, const char *text) // FTP 回应方法
{
	char buffer[MAX_BUFFER_SIZE] = {0};
	sprintf(buffer, "%d %s\r\n", code, text); // 格式化FTP回应
	send(sess->ctrl_fd, buffer, strlen(buffer), 0); // 发送给客户端
}

static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_pwd(session_t *sess);
static void do_type(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_list(session_t *sess);

typedef struct ftpcmd // 命令映射结构
{
	const char *cmd; // 命令
	void(*cmd_handler)(session_t *sess); //命令处理方法（函数指针）
}ftpcmd_t;

ftpcmd_t ctrl_cmds[] = // 命令结构体数组
{
	{"USER", do_user}, // 解析用户名
	{"PASS", do_pass}, // 解析密码
	{"SYST", do_syst}, // 获取系统信息
	{"FEAT", do_feat}, // 发送服务器支持的特性
	{"PWD" , do_pwd }, // 告诉用户当前目录是什么
	{"TYPE", do_type}, // 传输文件类型是文本还是二进制
	{"PORT", do_port}, // 服务器主动连接客户端
	{"PASV", do_pasv}, // 客户端主动连接服务器
	{"LIST", do_list}  // 显示资源列表
};

void handle_child(session_t *sess)//ftp 服务进程
{
    ftp_reply(sess, FTP_GREET, "(bit86 miniftp 1.0.1)"); // 发送欢迎信息给客户端 
    while(1)//不停的等待客户端的命令并做出处理
    {
			memset(sess->cmdline, 0, MAX_COMMOND_LINE_SIZE); // 将命令行数组的内容初始化为0
			memset(sess->cmd, 0, MAX_CMD_SIZE); // 命令数组初始化为0
			memset(sess->arg, 0, MAX_ARG_SIZE); // 参数数组初始化为0
	
			int ret = recv(sess->ctrl_fd, sess->cmdline, MAX_COMMOND_LINE_SIZE, 0); // 接收客户端发送的数据，Ctrl_fd是控制连接编码，接收的数据放到cmdline中
			//printf("cmdline = %s\n", sess->cmdline); // 查看接收的数据
			// ret是接收的命令行的长度
			if(ret < 0) // 如果接收数据失败，则输出错误信息
				ERR_EXIT("recv");
			if(ret == 0) // 如果没有接收到数据，则说明客户端已经关闭，直接退出进程
				exit(EXIT_SUCCESS);
			
			str_trim_crlf(sess->cmdline); // 剪切命令行
			str_split(sess->cmdline, sess->cmd, sess->arg, ' '); // 分割命令行
			//printf("cmd = %s\n", sess->cmd); // 显示解析出来的命令
			//printf("arg = %s\n", sess->arg); // 显示解析出来的参数
	
			//命令映射
			int table_size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]); // 计算命令结构体数组的长度
			int i;
			for(i=0; i<table_size; ++i) // 查找匹配解析出来的命令
			{
				if(strcmp(sess->cmd, ctrl_cmds[i].cmd) == 0) // 如果命令相同
				{
					if(ctrl_cmds[i].cmd_handler) // 如果命令对应的处理方法有效，则调用该方法
						ctrl_cmds[i].cmd_handler(sess); 
					else // 如果命令对应的方法没有实现，则服务器向客户端发送命令未识别响应
						ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command."); // 发送502信息（未定义方法）
					break; // 响应过客户端的命令后直接退出，不再查找
				}
			}
			if(i >= table_size) // 如果解析出来的命令不存在
				ftp_reply(sess, FTP_BADCMD, "Unknown command."); // 发送500信息（未知命令）
    }

}
// 比如客户端发送一个命令：USER lihua
static void do_user(session_t *sess) // 解析用户
{
	struct passwd *pwd = getpwnam(sess->arg); // arg是解析出来的参数（用户名），pwd保存的是用户的相关信息
	if(pwd != NULL) // 如果获取用户信息成功
		sess->uid = pwd->pw_uid; //保存用户ID
	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password"); // 发送331命令（请输入密码）
}

static void do_pass(session_t *sess) // 解析密码（通过用户的ID好判断密码）
{
	//鉴权登录
	struct passwd *pwd = getpwuid(sess->uid); // 通过用户的ID，获取用户信息
	if(pwd == NULL) // 如果用户不存在
	{
		//用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect."); // 发送530信息（登录失败）
		return;
	}
	struct spwd *spd = getspnam(pwd->pw_name); // 通过用户名，获取引证结构体
	if(spd == NULL) // 这里会不会多此一举？？
	{
		//用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect."); // 发送530信息（登录失败）
		return;
	}
	// 如果用户存在则判断密码, crypt是加密函数，arg是明文密码，sp_pwdp是掩码（加密密码）
	char *encrypted_pw = crypt(sess->arg, spd->sp_pwdp); // 将明文密码进行加密
	if(strcmp(encrypted_pw, spd->sp_pwdp) != 0)// 如果明文密码和加密密码不一样
	{
		//密码错误
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect."); // 发送530信息（登录失败）
		return;
	}

// 如果登录成功则会执行以下程序
	//更改ftp服务进程
	setegid(pwd->pw_gid); // 将root进程的孙子进程的群ID更改为用户的群ID
	seteuid(pwd->pw_uid); // 将root进程的孙子进程的ID更改为用户的ID
	chdir(pwd->pw_dir);  // 把当前工作目录更改为用户的宿主目录（用户家目录）

	ftp_reply(sess, FTP_LOGINOK, "Login successful."); // 发送230信息（登录成功）
}

static void do_syst(session_t *sess) // 获取系统信息
{
	ftp_reply(sess, FTP_SYSTOK, "Linux Type: L8"); // 发送215系统信息
}

static void do_feat(session_t *sess) // 告诉客户端，服务器的特性（即服务器支持哪些功能）
{
	send(sess->ctrl_fd, "211-Features:\r\n", strlen("211-Features:\r\n"), 0);
	send(sess->ctrl_fd, " EPRT\r\n", strlen(" EPRT\r\n"), 0);
	send(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"), 0); // 主动模式
	send(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"), 0);
	send(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"), 0); // 被动模式
	send(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"), 0); 
	send(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"), 0); // 可以计算字符的大小
	send(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"), 0);
	send(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"), 0); // 支持UTF8字符格式
	send(sess->ctrl_fd, "211 End\r\n", strlen("211 End\r\n"), 0);
}

static void do_pwd(session_t *sess) // 打印当前工作目录
{
	char cwd[MAX_CWD_SIZE] = {0}; // 
	getcwd(cwd, MAX_CWD_SIZE);   // 获取当前目录
	char text[MAX_BUFFER_SIZE] = {0};
	sprintf(text, "\"%s\"", cwd);
	ftp_reply(sess, FTP_MKDIROK, text); // 发送257代码（用户家目录信息）
}

static void do_type(session_t *sess) // 协商数据传输模式
{
	if(strcmp(sess->arg,"A")==0 || strcmp(sess->arg,"a")==0) // 如果文件类型为ASCII格式
	{
		sess->is_ascii = 1;
		ftp_reply(sess, FTP_TYPEOK, "Switching to ASCII mode."); // 发送200代码（文件类型格式正确）
	}
	else if(strcmp(sess->arg,"I")==0 || strcmp(sess->arg,"i")==0)
	{
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Switching to Binary mode."); // 发送200代码（文件类型格式正确）
	}
	else
	{
		//500 Unrecognised TYPE command.
		ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command."); // // 发送500信息（文件格式错误）
	}
}

/////////////////////////////////////////////////////////////////////
//数据协商
static void do_port(session_t *sess) // 协商主动连接
{
	//解析地址和端口号，假设客户端发送的命令为PORT 192,168,232,1,34,173
	unsigned int v[6] = {0};
	sscanf(sess->arg, "%u,%u,%u,%u,%u,%u", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]); // 分割IP和端口号，并将分割后的数字保存在数组v中
	
	sess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr)); // 开辟一个保存IP和端口的空间，然后将这个空间交给port_addr处理
	//填充协议家族
	sess->port_addr->sin_family = AF_INET; // 设置地址族为IPv4

	//填充port
	unsigned char *p = (unsigned char *)&(sess->port_addr->sin_port); // 设置地址的端口号信息
	p[0] = v[4];
	p[1] = v[5];

	//填充ip
	p = (unsigned char *)&(sess->port_addr->sin_addr); //　设置IP地址
	p[0] = v[0];
	p[1] = v[1];
	p[2] = v[2];
	p[3] = v[3];

	// 200 PORT command successful. Consider using PASV.
	ftp_reply(sess, FTP_PROTOK, "PORT command successful. Consider using PASV."); // 发送200代码（主动模式准备就绪）
}

static void do_pasv(session_t *sess) // 协商被动连接（如果客户端发送pasv命令，则执行这个函数）
{
	char ip[16] = "172.17.0.4"; // 服务器的IP
	unsigned int v[4] = {0};
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]); // 将IP分割


	int sockfd = tcp_server(ip, 0);	//服务器处于监听状态，0代表生成的默认端口号，sockfd是监听套接字

	struct sockaddr_in addr; // 地址结构体
	socklen_t addrlen = sizeof(struct sockaddr); // 地址长度
	if(getsockname(sockfd,	(struct sockaddr*)&addr, &addrlen) < 0) // 如果获取套接字的名字失败
		ERR_EXIT("getsockname");

	sess->pasv_listen_fd = sockfd; // 将被动连接套接字保存在会话结构体中

	unsigned short port = ntohs(addr.sin_port); // 获取端口号，sin_port是网络字节序，ntohs可以将sin_port转化为本地字节序

	char text[MAX_BUFFER_SIZE] = {0};
	sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
		v[0],v[1],v[2],v[3], port>>8, port&0x00ff); // 将地址和端口号格式化

	//227 Entering Passive Mode (192,168,232,10,248,159).
	ftp_reply(sess, FTP_PASVOK, text); // 发送227代码（被动连接成功），服务器将自己的IP和端口号发送给客户端
}

////////////////////////////////////////////////////////////////////////
//数据连接

int port_active(session_t *sess) //主动模式连接
{
	if(sess->port_addr != NULL) // 如果主动模式被激活，则返回1，否则返回0
		return 1;
	return 0;
}

int pasv_active(session_t *sess) // 被动连接模式
{
	if(sess->pasv_listen_fd != -1) // 如果被动连接被激活，则返回1，否则返回0
		return 1;
	return 0;
}

static int get_transfer_fd(session_t *sess) // 获取数据连接的套接字
{
	// 首先选择主动连接或被动连接
	if(!port_active(sess) && !pasv_active(sess)) // 如果主动和被动模式都没有被激活
	{
		//425 Use PORT or PASV first.
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first."); // 发送425代码（连接失败，提示客户端重新发送PORT或PASV命令建立连接）
		return -1;
	}

	if(port_active(sess)) // 如果主动连接被激活
	{
		int sock = tcp_client(); // 创建数据流套接字
		socklen_t addrlen = sizeof(struct sockaddr); // 地址长度
		if(connect(sock, (const struct sockaddr*)sess->port_addr, addrlen) < 0) // 服务器主动连接客户端，套接字在这里被使用
			return -1;
		//保存数据连接套接字
		sess->data_fd = sock; // 将被激活的套接字保存
	}
	if(pasv_active(sess)) // 如果被动连接被激活
	{
		int sockConn;
		struct sockaddr_in addr; // 地址结构
		socklen_t addrlen; // 地址长度
		if((sockConn = accept(sess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen)) < 0) // 服务器接受客户端的连接，并产成一个用于数据传输的套接字sockConn
			return -1;
		sess->data_fd = sockConn;  // 将被激活的数据连接套接字保存
	}

	if(sess->port_addr) // 释放地址结构空间
	{
		free(sess->port_addr); // 释放IP地址和端口的空间，防止内存泄露（因为已经连接成功，所以IP没有用了）
		sess->port_addr = NULL;
	}
	return 0;
}

//drwxrwxr-x    2 1000     1000          114 Dec 05  2020 93
void list_common(session_t *sess) // 发送文件列表数据
{
	DIR *dir = opendir("."); // 打开当前目录（用户家目录）
	if(dir == NULL) // 打开目录失败
		ERR_EXIT("opendir");

	struct stat sbuf;
	char   buf[MAX_BUFFER_SIZE] = {0};
	unsigned int offset = 0; //offset字符偏移量

	struct dirent *dt;
	while((dt = readdir(dir))) // 循环读取目录中的内容，直到内容读取完
	{
		if(stat(dt->d_name,  &sbuf)<0)  // stat函数可以根据文件路径可以获取文件信息，并将文件信息放在sbuf中
			ERR_EXIT("stat");

		if(dt->d_name[0] == '.')  // 过滤掉隐藏文件（不显示隐藏文件）
			continue;
	// 将文件信息组合起来，并保存在buf中
		const char *perms = statbuf_get_perms(&sbuf); // 获取文件权限
		offset = sprintf(buf, "%s", perms); // 将权限字符串数据写入到buf中

		offset += sprintf(buf+offset, "%3d %-8d %-8d %8u ",  
			(int)sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid, (unsigned int)sbuf.st_size);

		const char *pdate = statbuf_get_date(&sbuf); // 获取文件的时间信息
		offset += sprintf(buf+offset, "%s ", pdate); // 将文件的时间信息保存在buf中

		sprintf(buf+offset, "%s\r\n", dt->d_name); // 将文件名字保存在buf中。\r\n是要求的格式，不可省略 
		
		//最终发送给客户端的信息：drwxrwxr-x    2 1000     1000          114 Dec 05  2020 93 

		send(sess->data_fd, buf, strlen(buf), 0); // 将buf中的内容通过data_fd套接字发送给客户端
	}

	closedir(dir); // 关闭目录
}

static void do_list(session_t *sess) // 显示资源列表
{
	//1 创建数据连接
	if(get_transfer_fd(sess) != 0) // 获取数据连接，如果返回值不为0，则说明失败
		return;

	//2 150
	ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing."); // 发送150代码（数据连接成功，即将显示资源列表）

	//3 传输列表
	list_common(sess);

	//4 226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK."); // 发送226代码（资源列表数据传输成功）

	//关闭数据连接
	close(sess->data_fd); //关闭数据传输套接字
	sess->data_fd = -1; // 处于无连接状态
}