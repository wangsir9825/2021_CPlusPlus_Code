#include"str.h"

void str_trim_crlf(char *str) // 字符串剪切，去掉回车和换行
{
	char *p = str + (strlen(str)-1);
	while(*p=='\r' || *p=='\n')
		*p-- = '\0'; // 这里很妙
}

 // 字符串分割，将命令、参数根据指示（token）进行分割，命令保存在left中，参数保存在right中
void str_split(const char *str, char *left, char *right, char token)
{
	char *pos = strchr(str, token); // 查找空格
	if(pos == NULL) // 如果没有找到空格
		strcpy(left, str);
	else
	{
		strncpy(left, str, pos-str);
		strcpy(right, pos+1); // 将第二个字符串赋值给right
	}
}

void str_upper(char *str) // 小写字符串转化为大写
{
	while(*str != '\0')
	{
		if(*str>='a' && *str<='z')
			*str -= 32;
		str++;
	}
}