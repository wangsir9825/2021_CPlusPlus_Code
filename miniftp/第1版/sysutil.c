#include"sysutil.h"

int tcp_server(const char *host, unsigned short port)// 创建套接字
{
    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//创建套接字错误
        ERR_EXIT("socket");

    struct sockaddr_in addrSer;// 绑定地址
    addrSer.sin_family = AF_INET;
    addrSer.sin_port = htons(port);
    addrSer.sin_addr.s_addr = inet_addr(host);
    //设置地址重用，防止程序地址被占用 
    int on = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");

    if(bind(listenfd, (struct sockaddr*)&addrSer, sizeof(addrSer)) < 0)//绑定监听套接字
        ERR_EXIT("bind");

    if(listen(listenfd, SOMAXCONN) < 0)//监听
        ERR_EXIT("listen");

    return listenfd;

}
