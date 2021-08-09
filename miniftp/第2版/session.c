
#include"session.h"
#include"ftpproto.h"
#include"privparent.h"

void begin_session(session_t *sess)//会话功能
{
    pid_t pid = fork();
    if(pid == -1)
        ERR_EXIT("session fork");

    if(pid == 0)
    {

        handle_child(sess);//ftp 服务进程 

    }
    else //nobody 进程，nobody是系统中比较特殊的用户，主要用于协助ftp服务进程（孙子进程），是内部私有的进程 
    {
				// 更改nobody进程名
				struct passwd *pwd = getpwnam("nobody"); //获取nobody相关信息
				if(pwd == NULL)
					ERR_EXIT("getpwnam");
				if(setegid(pwd->pw_gid) < 0) // 将root子进程的群ID更改为nobody的群ID如果失败则报错
					ERR_EXIT("setegid");
				if(seteuid(pwd->pw_uid) < 0) // 将root子进程的ID更改为nobody的ID如果失败则报错
					ERR_EXIT("seteuid");
				
        handle_parent(sess);

    }

}
