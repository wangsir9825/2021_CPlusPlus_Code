#include"ftpproto.h"
#include"session.h"
#include"str.h"
#include"ftpcodes.h"

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
	{"PORT", do_port}, // 客户端主动连接服务器
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
			if(ret == 0) // 如果没有接收到数据，则说明服务器已经关闭，直接退出进程
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

static void do_pass(session_t *sess) // 解析密码
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

	//更改ftp服务进程
	setegid(pwd->pw_gid); // 将root进程的孙子进程的群ID更改为用户的群ID
	seteuid(pwd->pw_uid); // 将root进程的孙子进程的ID更改为用户的ID
	chdir(pwd->pw_dir);  // 把当前工作目录更改为用户的宿主目录（用户家目录）

	ftp_reply(sess, FTP_LOGINOK, "Login successful."); // 发送230信息（登录成功）
}

static void do_syst(session_t *sess)
{
	ftp_reply(sess, FTP_SYSTOK, "Linux Type: L8"); // 发送215系统信息
}

static void do_feat(session_t *sess) // 服务器特性
{
	send(sess->ctrl_fd, "211-Features:\r\n", strlen("211-Features:\r\n"), 0);
	send(sess->ctrl_fd, " EPRT\r\n", strlen(" EPRT\r\n"), 0);
	send(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"), 0);
	send(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"), 0);
	send(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"), 0);
	send(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"), 0);
	send(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"), 0);
	send(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"), 0);
	send(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"), 0);
	send(sess->ctrl_fd, "211 End\r\n", strlen("211 End\r\n"), 0);
}

static void do_pwd(session_t *sess) // 打印当前目录
{
	char cwd[MAX_CWD_SIZE] = {0}; // 
	getcwd(cwd, MAX_CWD_SIZE);   // 获取当前目录
	char text[MAX_BUFFER_SIZE] = {0};
	sprintf(text, "\"%s\"", cwd);
	ftp_reply(sess, FTP_MKDIROK, text); // 发送257代码（用户家目录信息）
}

static void do_type(session_t *sess) // 文件类型
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

// 建立数据连接
static void do_port(session_t *sess) // 主动连接
{
	//解析地址和端口号
}

static void do_list(session_t *sess) // 显示资源列表
{
	//1 创建数据连接

	//2 150

	//3 传输列表

	//4 226
}