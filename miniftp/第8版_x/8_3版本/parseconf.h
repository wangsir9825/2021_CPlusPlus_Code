#ifndef _PARSE_CONF_H_ // 解析模块
#define _PARSE_CONF_H_

#include"common.h"

void parseconf_load_file(const char *path); // 加载配置文件，即从配置文件中读取信息
void parseconf_load_setting(const char*setting); // 解析配置文件，每次解析一行，循环解析，所以称为解析配置行更贴切

#endif /* _PARSE_CONF_H_ */