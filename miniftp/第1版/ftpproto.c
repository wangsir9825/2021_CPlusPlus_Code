#include"ftpproto.h"

void handle_child(session_t *sess)//ftp 服务进程
{
    send(sess->ctrl_fd, "220 (miniftp 1.0.1)\r\n", strlen("220 (miniftp 1.0.1)\r\n"), 0);//服务器给客户端一个响应
    while(1)//不停的等待客户端的命令并做出处理
    {


    }


}
