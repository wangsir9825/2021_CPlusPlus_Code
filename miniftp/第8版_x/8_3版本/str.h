// �ַ���ģ�飬���ڽ������յ�������
#ifndef _STR_H_
#define _STR_H_

#include"common.h"

void str_trim_crlf(char *str); // �ַ������У�ȥ���س��ͻ���
void str_split(const char *str, char *left, char *right, char token); // �ַ����ָ�������������ָʾ��token�����зָ�
void str_upper(char *str); // Сд�ַ���ת��Ϊ��д

#endif /* _STR_H_ */