#include"str.h"

void str_trim_crlf(char *str) // �ַ������У�ȥ���س��ͻ���
{
	char *p = str + (strlen(str)-1);
	while(*p=='\r' || *p=='\n')
		*p-- = '\0'; // �������
}

 // �ַ����ָ�������������ָʾ��token�����зָ�������left�У�����������right��
void str_split(const char *str, char *left, char *right, char token)
{
	char *pos = strchr(str, token); // ���ҿո�
	if(pos == NULL) // ���û���ҵ��ո�
		strcpy(left, str);
	else
	{
		strncpy(left, str, pos-str);
		strcpy(right, pos+1); // ���ڶ����ַ�����ֵ��right
	}
}

void str_upper(char *str) // Сд�ַ���ת��Ϊ��д
{
	while(*str != '\0')
	{
		if(*str>='a' && *str<='z')
			*str -= 32;
		str++;
	}
}