#ifndef _PRIV_SOCK_H_ // ˽��ͨ��ģ�飬����ftp���̺�nobody���̽���ͨ��
#define _PRIV_SOCK_H_

#include"common.h"
#include"session.h"

//FTP���������nobody�������������
#define PRIV_SOCK_GET_DATA_SOCK 1 // �����ȡ���������׽���
#define PRIV_SOCK_PASV_ACTIVE 2 // ��ȡ����ģʽ�Ƿ񱻼���
#define PRIV_SOCK_PASV_LISTEN 3 // ����ģʽ�ļ����׽���
#define PRIV_SOCK_PASV_ACCEPT 4 // �����������

//nobody ���̶�FTP������̵�Ӧ��
#define PRIV_SOCK_RESULT_OK 1 // Ӧ��ɹ�
#define PRIV_SOCK_RESULT_BAD 2 // Ӧ��ʧ��

// �ڲ�ͨ�ŷ���
void priv_sock_init(session_t *sess); // ��ʼ���ڲ����̼�ͨ��ͨ�� 
void priv_sock_close(session_t *sess); // �ر��ڲ����̼�ͨ��ͨ��
void priv_sock_set_parent_context(session_t *sess); // ���ø����̻���
void priv_sock_set_child_context(session_t *sess); // �����ӽ��̻���
void priv_sock_send_cmd(int fd, char cmd); // ��������(�ӡ�>��)
char priv_sock_get_cmd(int fd); // ��������(��<-- ��) 
void priv_sock_send_result(int fd, char res); // ���ͽ��(����>��)
char priv_sock_get_result(int fd); // ���ս��(��<--��) 
void priv_sock_send_int(int fd, int the_int); // ����һ������
int priv_sock_get_int(int fd); // ����һ������
void priv_sock_send_buf(int fd, const char *buf, unsigned int len); // ����һ���ַ���
void priv_sock_recv_buf(int fd, char *buf, unsigned int len); // ����һ���ַ���
void priv_sock_send_fd(int sock_fd, int fd); // �����ļ�������
int priv_sock_recv_fd(int sock_fd); // �����ļ�������


#endif /* _PRIV_SOCK_H_ */