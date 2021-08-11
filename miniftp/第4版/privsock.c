#include"privsock.h" // ͨ��ģ���ʵ��
#include"sysutil.h"

void priv_sock_init(session_t *sess) // ��ʼ���ڲ����̼�ͨ��ͨ�� 
{
	int sockfds[2];
	if(socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0) // ����ȫ˫��sockfds,�൱��ȫ˫���ܵ����ṹ��PF_UNIX��SOCK_STREAM������ʽ�׽���
		ERR_EXIT("socketpair");
	sess->parent_fd = sockfds[0]; // �߱���дȨ�޵�ͨ��
	sess->child_fd = sockfds[1];
}

void priv_sock_close(session_t *sess) // �ر��ڲ����̼�ͨ��ͨ��
{
	if(sess->parent_fd != -1)
	{
		close(sess->parent_fd);
		sess->parent_fd = -1;
	}
	if(sess->child_fd != -1)
	{
		close(sess->child_fd);
		sess->child_fd = -1;
	}
}

void priv_sock_set_parent_context(session_t *sess) // ���ø����������Ļ���
{
	if(sess->child_fd != -1) // �ر��ӽ��̵�ͨ��ͨ��
	{
		close(sess->child_fd );
		sess->child_fd = -1;
	}
}

void priv_sock_set_child_context(session_t *sess) // �����ӽ��������Ļ���
{
	if(sess->parent_fd != -1)
	{
		close(sess->parent_fd); // �رո����̵�ͨ��ͨ��
		sess->parent_fd = -1;
	}
}

void priv_sock_send_cmd(int fd, char cmd) // ��������(�ӡ�>��)
{
	int ret = send(fd, &cmd, sizeof(cmd), 0); // ftp����ͨ��fd��cmd����͸�nobody���̣�0������ģʽĬ�ϵķ�ʽ��retΪ����ĳ���
	if(ret != sizeof(cmd))
		ERR_EXIT("priv_sock_send_cmd");
}

char priv_sock_get_cmd(int fd) // ��������(��<-- ��) 
{
	printf("wwwwwwwwwwwww\n");
	char cmd;
	int ret = recv(fd, &cmd, sizeof(cmd), 0); // �����ȴ����������fdͨ������cmd������շ�ʽΪĬ�ϣ�retΪftp���̽��յ�����ĳ���
	if(ret == 0) // retΪ0˵��ftp����û�н��յ�����
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(cmd)) // ret����������ĳ��ȣ�˵��ftp�����������
	{
		printf("aaaaaaaaaaaa\n");
		ERR_EXIT("priv_sock_get_cmd");
	}
	return cmd; // �ɹ��������
}

void priv_sock_send_result(int fd, char res) // ���ͽ��(����>��)
{
	int ret = send(fd, &res, sizeof(res), 0);
	if(ret != sizeof(res))
		ERR_EXIT("priv_sock_send_result");
}

char priv_sock_get_result(int fd) // ���ս��(��<--��) 
{
	char res;
	int ret = recv(fd, &res, sizeof(res), 0);
	if(ret == 0)
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(res))
		ERR_EXIT("priv_sock_get_result");
	return res;
}

void priv_sock_send_int(int fd, int the_int) // ����һ������
{
	int ret = send(fd, &the_int, sizeof(the_int), 0);
	if(ret != sizeof(the_int))
		ERR_EXIT("priv_sock_send_int");
}

int priv_sock_get_int(int fd)// ����һ������
{
	int the_int;
	int ret = recv(fd, &the_int, sizeof(the_int), 0);
	if(ret == 0)
	{
		printf("ftp process exit\n");
		exit(EXIT_SUCCESS);
	}
	if(ret != sizeof(the_int))
		ERR_EXIT("priv_sock_get_int");
	return the_int;
}

void priv_sock_send_buf(int fd, const char *buf, unsigned int len) // ����һ���ַ���
{
	priv_sock_send_int(fd, len); // �ֽ����ķ��ͷ�ʽû�취�ж��ַ����ĳ��ȣ������ȸ��߶Է��ַ����ĳ���
	int ret = send(fd, buf, len, 0);
	if(ret != len)
		ERR_EXIT("priv_sock_send_buf");
}

void priv_sock_recv_buf(int fd, char *buf, unsigned int len) // ����һ���ַ��������len�Ǵ���buf�ĳ���
{
	int recv_len = priv_sock_get_int(fd); // �����ַ����ĳ���
	if(recv_len > len) // ������յ��ĳ��ȱȻ������ĳ��Ȼ�Ҫ����˵�����������
		ERR_EXIT("priv_sock_recv_buf");

	int ret = recv(fd, buf, len, 0); // ��ͨ��fd���յ����ݱ�����buf��
	if(ret != recv_len) // �����ʵ���յ������ݳ���ret�����ڶԷ����͵����ݳ���
		ERR_EXIT("priv_sock_recv_buf");
}

void priv_sock_send_fd(int sock_fd, int fd) // �����ļ�������
{
	send_fd(sock_fd, fd); // ͨ��sock_fdͨ������fd�׽��֣���Ҫ���������Ļ���Ҳ���͹�ȥ
}

int priv_sock_recv_fd(int sock_fd) // �����ļ�������
{
	return recv_fd(sock_fd); // ͨ��sock_fdͨ������fd�׽���
}
