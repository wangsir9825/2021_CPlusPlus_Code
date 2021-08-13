#include"ftpproto.h"
#include"session.h"
#include"str.h"
#include"ftpcodes.h"
#include"sysutil.h"
#include"privsock.h"

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
static void do_cwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_size(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_rest(session_t *sess);

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
	{"LIST", do_list}, // 显示资源列表
	{"CWD" , do_cwd }, // 更改当前目录
	{"MKD" , do_mkd }, // 新建文件夹
	{"RMD" , do_rmd }, // 删除目录
	{"DELE", do_dele}, // 删除文件
	{"SIZE", do_size}, // 查看文件的大小（单位：字节）
	// 重命名
	{"RNFR", do_rnfr}, // 重命名谁？
	{"RNTO", do_rnto}, // 重命名后的名字是？
	{"RETR", do_retr}, // 下载文件
	{"STOR", do_stor}, // 上传文件
	{"REST", do_rest}	// 断点续传
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
	unsigned char *p = (unsigned char *)&(sess->port_addr->sin_port); // 保存客户端的端口号信息
	p[0] = v[4];
	p[1] = v[5];

	//填充ip
	p = (unsigned char *)&(sess->port_addr->sin_addr); //　保存客户端的IP地址
	p[0] = v[0];
	p[1] = v[1];
	p[2] = v[2];
	p[3] = v[3];

	// 200 PORT command successful. Consider using PASV.
	ftp_reply(sess, FTP_PROTOK, "PORT command successful. Consider using PASV."); // 发送200代码（主动模式准备就绪）
}

static void do_pasv(session_t *sess) // 协商被动连接（如果客户端发送pasv命令，则执行这个函数）
{ 
	// ftp进程向nobody进程发送请求被动连接命令
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);
	
	char ip[16] = {0};

	//接收服务器的IP
	int len = priv_sock_get_int(sess->child_fd); // 接收IP字符串长度
	priv_sock_recv_buf(sess->child_fd, ip, len); // 接收IP字符串
	//接收服务器的port
	unsigned short port = (unsigned short)priv_sock_get_int(sess->child_fd);

	
	unsigned int v[4] = {0};
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]); // 将IP分割（或称将IP格式化）
	char text[MAX_BUFFER_SIZE] = {0};
	sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
		v[0],v[1],v[2],v[3], port>>8, port&0x00ff); // 将地址和端口号格式化

	//227 Entering Passive Mode (192,168,232,10,248,159).
	ftp_reply(sess, FTP_PASVOK, text); // 发送227代码（被动连接成功），服务器将自己的IP和端口号发送给客户端
}

////////////////////////////////////////////////////////////////////////
//数据连接
int pasv_active(session_t *sess); // 因为port_active会用到pasv_active所以需要声明
int port_active(session_t *sess) //主动连接模式被激活，则返回1，否则返回0
{
	if(sess->port_addr != NULL)
	{
		if(pasv_active(sess)) // 如果在主动模式被激活的前提下，被动模式也被激活，则报错（我们不允许两种模式同时被激活）
			ERR_EXIT("both port an pasv are active");
		return 1;
	}
	return 0;
}

int pasv_active(session_t *sess) // 如果被动连接模式被激活，则返回1，否则返回0
{
	// ftp进程向nobody进程发送请求判断被动模式是否被激活
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACTIVE);
	
	int active = priv_sock_get_int(sess->child_fd);
	if(active != -1)
	{
		if(port_active(sess)) // 如果在被动模式被激活的前提下，主动模式也被激活，则报错（不允许两种模式同时被激活）
			ERR_EXIT("both port an pasv are active");
		return 1;
	}
	return 0;
}
int get_port_fd(session_t *sess) // 获取主动连接的套接字
{
	// ftp进程向nobody进程发送请求主动连接命令
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK); 
	
	//发送给nobody进程ip地址（没有客户端的IP地址，nobody进程无法完成主动连接）
	char *ip = inet_ntoa(sess->port_addr->sin_addr); // inet_ntoa可以将sin_addr转换为字符串
	priv_sock_send_int(sess->child_fd, strlen(ip)); // 通过child_fd通道发送字符串的长度
	priv_sock_send_buf(sess->child_fd, ip, strlen(ip));  // 通过child_fd通道发送IP字符串

	//发送port
	unsigned short port = ntohs(sess->port_addr->sin_port); // 将一个16位数端口由网络字节顺序转换为主机字节顺序
	priv_sock_send_int(sess->child_fd, (int)port); // 通过child_fd通道发送端口号

	// 接收nobody进程的应答
	char res = priv_sock_get_result(sess->child_fd); 
	if(res == PRIV_SOCK_RESULT_BAD)// PRIV_SOCK_RESULT_BAD表示请求连接失败
		return -1;

	sess->data_fd = priv_sock_recv_fd(sess->child_fd); // 接收数据连接套接字
	return 0;
}

int get_pasv_fd(session_t *sess) // 获取被动连接的套接字
{
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT); // PRIV_SOCK_PASV_ACCEPT在被动模式下请求被动连接
	char res = priv_sock_get_result(sess->child_fd); // 接收nobody的应答
	if(res == PRIV_SOCK_RESULT_BAD)
		return -1;

	sess->data_fd = priv_sock_recv_fd(sess->child_fd); // 
	return 0;
}

static int get_transfer_fd(session_t *sess) // 获取数据连接的套接字
{
	// 首先选择主动连接或被动连接
	if(!port_active(sess) && !pasv_active(sess)) // 如果主动和被动模式都没有被激活
	{
		//425 Use PORT or PASV first.
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first."); //发送425代码（连接失败，提示客户端重新发送PORT或PASV命令建立连接）
		return -1;
	}

	if(port_active(sess))
	{
		if(get_port_fd(sess) != 0) // 如果获取主动连接套接字失败
			return -1;
	}
	if(pasv_active(sess))
	{
		if(get_pasv_fd(sess) != 0) // 如果获取被动连接套接字失败
			return -1;
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

static void do_cwd(session_t *sess) // 更改当前目录，实现CD命令
{
	if(chdir(sess->arg) < 0) // 如果切换目录失败
		ftp_reply(sess, FTP_NOPERM, "Failed to change directory."); // 发送550代码（更改目录失败，失败的原因可能是没有权限或响应失败）
	else
		ftp_reply(sess, FTP_CWDOK, "Directory successfully changed."); // 发送25 0代码（更改目录成功）
}

static void do_mkd(session_t *sess) // 新建文件夹
{
	if(mkdir(sess->arg, 0755) < 0) // 如果新建文件夹失败（arg是新建的文件夹名，0775是权限）
		ftp_reply(sess, FTP_NOPERM, "Create directory operation failed."); // 发送550代码（新建目录失败）
	else
	{
		char text[MAX_BUFFER_SIZE] = {0};
		sprintf(text, "\"%s\" created", sess->arg); // 将信息格式化
		ftp_reply(sess, FTP_MKDIROK, text); // 发送257代码（新建目录成功）257 "/home/lihua/heheda" created
	}
}

static void do_rmd(session_t *sess) // 删除目录
{
	if(rmdir(sess->arg) < 0) // 如果删除目录失败
		ftp_reply(sess, FTP_FILEFAIL, "Remove directory operation failed."); // 发送550代码
	else
	{
		ftp_reply(sess, FTP_RMDIROK, "Remove directory operation successful."); // 发送250代码
	}
}

static void do_dele(session_t *sess) // 删除文件
{
	if(unlink(sess->arg) < 0) // 如果删除文件失败
		ftp_reply(sess, FTP_NOPERM, "Delete operation failed."); // 发送550代码
	else
		ftp_reply(sess, FTP_DELEOK, "Delete operation successful."); // 发送250代码
}

static void do_size(session_t *sess) // 查看文件的大小（单位：字节）
{
	struct stat sbuf;
	if(stat(sess->arg, &sbuf) < 0) // 如果查看文件的大小失败
		ftp_reply(sess, FTP_FILEFAIL, "Could not get file size."); // 发送550代码（无法获取文件的大小，失败的原因没有该文件或响应失败）
	else
	{
		char text[MAX_BUFFER_SIZE] = {0};
		sprintf(text, "%d", (int)sbuf.st_size); // 将文件大小信息格式化到text文件中
		ftp_reply(sess, FTP_SIZEOK, text); // 发送257代码
	}
}

static void do_rnfr(session_t *sess) // 重命名谁？
{
	unsigned int len = strlen(sess->arg); // 接收旧名字
	sess->rnfr_name = (char*)malloc(len + 1); // 保存旧名字之开辟空间
	memset(sess->rnfr_name, 0, len+1); // 空间的值初始化为0
	strcpy(sess->rnfr_name, sess->arg); // 把旧名字放到空间中
	ftp_reply(sess, FTP_RNFROK, "Ready for RNTO."); // 发送350代码（重命名已经准备好）
}

static void do_rnto(session_t *sess) //重命名后的文件名是？
{
	if(sess->rnfr_name == NULL) // 如果没有响应RNFR命令
	{
		ftp_reply(sess, FTP_NEEDRNFR, "RNFR required first."); // 发送503代码（提示需要先执行RNFR命令）
		return;
	}
	if(rename(sess->rnfr_name, sess->arg) < 0) // 如果重命名失败
	{
		ftp_reply(sess, FTP_NOPERM, "Rename failed."); //  发送550代码（重命名失败）
	}
	else
	{
		free(sess->rnfr_name); // 释放文件名的空间
		sess->rnfr_name = NULL;
		ftp_reply(sess, FTP_RENAMEOK, "Rename successful."); //  发送250代码（重命名成功）
	}
}

static void do_retr(session_t *sess) // 下载文件
{
	if(get_transfer_fd(sess) != 0) // 如果建立数据连接失败
		return;

	int fd;
	if((fd = open(sess->arg, O_RDONLY)) < 0) // 如果打开文件失败（在客户端打开一个文件）
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to open file."); // 发送xx代码（打开文件失败）
		return;
	}
	//回复150
	struct stat sbuf; // 状态结构，用于获取文件大小
	fstat(fd, &sbuf); // 将fd对应的文件的状态信息保存在sbuf中，这样文件的大小信息也被保存在sbuf里面了
	char buf[MAX_BUFFER_SIZE] = {0};
	if(sess->is_ascii) // 如果是ASCII模式传输，响应信息如下
		sprintf(buf, "Opening ASCII mode data connection for %s (%u bytes).", sess->arg, (unsigned int)sbuf.st_size);
	else // 如果是二进制模式传输，响应信息如下
		sprintf(buf, "Opening BINARY mode data connection for %s (%u bytes).",sess->arg, (unsigned int)sbuf.st_size);
	ftp_reply(sess, FTP_DATACONN, buf); // 发送150代码（打开文件成功，数据传输链接创建成功）

	 // 如果传输的文件很大
	unsigned int total_size = sbuf.st_size; // 文件总大小
	int read_count = 0; // 每次读取的文件大小
	while(1) // 如果文件很大，这时候需要循环传输，每次传输的数据大小为buf（文件切片传输）
	{
		memset(buf, 0, MAX_BUFFER_SIZE); // 初始化缓冲区
		read_count = total_size > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : total_size; // 读取的数据大小最大不超过缓冲区buf的空间的容量
		int ret = read(fd, buf, read_count); // 读取文件中的数据，把读取的数据保存在buf中，每次读取的数据大小为read_count
		if(ret==-1 || ret!=read_count) // 如果没有读取到数据，或者读取的数据有错误（包括漏读或多读）
		{
			ftp_reply(sess, FTP_BADSENDNET, "Failure writting to network stream."); // 发送426代码（读取数据失败）
			break; // 只要出错，就中断传输
		}
		if(ret == 0) // 如果数据被读取完毕
		{
			ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete."); // 发送226代码（传输文件完毕）
			break; // 传输完毕，退出循环
		}
		send(sess->data_fd, buf, ret, 0); // 把buf中的数据发送给客户端
		total_size -= read_count; // 计算剩余的数据大小
	}

	close(fd); // 关闭文件
	if(sess->data_fd != -1) // 只有文件传输通道处于连接状态，才能关闭数据传输通道
	{
		close(sess->data_fd); // 关闭数据传输通道
		sess->data_fd = -1;
	}
}

static void do_stor(session_t *sess) // 上传文件
{
	if(get_transfer_fd(sess) != 0) // 如果建立数据连接失败
		return;

	int fd;
	if((fd = open(sess->arg, O_CREAT|O_WRONLY, 0755)) < 0) // 如果在服务器上打开一个文件失败
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to open file."); // 发送550代码（上传文件失败）
		return;
	}

	//回复150
	ftp_reply(sess, FTP_DATACONN, "Ok to send data."); // 发送150代码（数据传输连接创建成功）

	//断点续传
	unsigned long long offset = sess->restart_pos; // 偏移量
	sess->restart_pos = 0;
	if(lseek(fd, offset, SEEK_SET) < 0) // 如果跳转到续传位置失败（相对起始位置偏移量为offset）
	{
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file."); // 发送553代码（无法继续续传）
		return;
	}

	//传输数据
	char buf[MAX_BUFFER_SIZE] = {0}; // 
	while(1)
	{
		memset(buf, 0, MAX_BUFFER_SIZE); // 初始化缓冲区
		int ret = recv(sess->data_fd, buf, MAX_BUFFER_SIZE, 0); // 接收数据
		if(ret == -1) // 读取失败
		{
			ftp_reply(sess, FTP_BADSENDNET, "Failure writting to network stream."); // 发送xx代码（传输失败，网络有问题）
			break;
		}
		if(ret == 0) // 传输完毕
		{
			ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete."); // 传输完毕
			break;
		}
		write(fd, buf, ret); // 将数据写入到文件中
	}

	close(fd); // 关闭文件
	if(sess->data_fd != -1)
	{
		close(sess->data_fd); // 关闭数据连接通道
		sess->data_fd = -1;
	}
}
//断点续传（上传和下载）
static void do_rest(session_t *sess)
{
	sess->restart_pos = (unsigned long long)atoll(sess->arg); // 保存续传位置，atoll将字符串转化为长长整型
	//350 Restart position accepted (3465248768).
	char text[MAX_BUFFER_SIZE] = {0}; // 
	sprintf(text, "Restart position accepted (%lld).", sess->restart_pos); // 组合信息
	ftp_reply(sess, FTP_RESTOK, text); // 发送350代码（续传的位置被接受）
}