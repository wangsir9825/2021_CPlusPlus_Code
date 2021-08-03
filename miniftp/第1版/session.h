#ifndef _SESSION_H_//建立会话模块
#define _SESSION_H_

#include"common.h"


typedef struct session//会话结构体
{

    int ctrl_fd;//控制连接
}session_t;

void begin_session(session_t *sess);//开始交谈

#endif /* _SESSION_H_ */
