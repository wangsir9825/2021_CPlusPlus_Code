
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
    else
    {

        handle_parent(sess);//nobody 进程，nobody是系统中比较特殊的用户，这个用户不允许登录，主要用于协助ftp服务进程，是内部私有的进程 

    }

}
