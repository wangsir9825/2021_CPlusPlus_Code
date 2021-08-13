#include"session.h"
#include"ftpproto.h"
#include"privparent.h"
#include"privsock.h"

void begin_session(session_t *sess)//会话功能
{
		priv_sock_init(sess);
    pid_t pid = fork();
    if(pid == -1)
        ERR_EXIT("session fork");

    if(pid == 0)
    {
				priv_sock_set_child_context(sess); // 设置ftp进程的上下文环境
        handle_child(sess);//ftp 服务进程 
    }
    else //nobody 进程，nobody是系统中比较特殊的用户，主要用于协助ftp服务进程（孙子进程），是内部私有的进程 
    {
				priv_sock_set_parent_context(sess); // 设置nobody进程的上下文环境
        handle_parent(sess);
    }

}
