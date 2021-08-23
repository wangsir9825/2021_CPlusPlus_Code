// 字符串模块，用于解析接收到的命令
#ifndef _STR_H_
#define _STR_H_

#include"common.h"

void str_trim_crlf(char *str); // 字符串剪切，去掉回车和换行
void str_split(const char *str, char *left, char *right, char token); // 字符串分割，将命令、参数根据指示（token）进行分割
void str_upper(char *str); // 小写字符串转化为大写

#endif /* _STR_H_ */