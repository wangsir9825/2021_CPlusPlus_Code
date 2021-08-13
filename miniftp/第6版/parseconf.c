#include"parseconf.h"
#include"tunable.h"
#include"str.h"


//bool����������ṹ������
static struct parseconf_bool_setting 
{
	const char *p_setting_name; //�����������
	int        *p_variable;     //�������ֵ����һ������ָ�����ָ�����ֵ��
}
parseconf_bool_array[] = // ����һ��bool����������ṹ����������
{
	{"pasv_enable", &tunable_pasv_enable}, // �Ƿ�������ģʽ
	{"port_enable", &tunable_port_enable}, // �Ƿ�������ģʽ
	{NULL, NULL} // �ÿ�ָ��������Ϊ������־
};

//int����������ṹ������
static struct parseconf_uint_setting
{
	const char   *p_setting_name;
	unsigned int *p_variable;
}
parseconf_uint_array[] = 
{
	{"listen_port", &tunable_listen_port}, // �˿�
	{"max_clients", &tunable_max_clients}, // ���������
	{"max_per_ip" , &tunable_max_per_ip}, // ÿip���������
	{"accept_timeout", &tunable_accept_timeout}, // accept��ʱʱ��
	{"connect_timeout", &tunable_connect_timeout}, // connect��ʱʱ��
	{"idle_session_timeout", &tunable_idle_session_timeout}, // �������ӳ�ʱ�ȴ�ʱ��
	{"data_connection_timeout", &tunable_data_connection_timeout}, // ���ݴ��䳬ʱ�ȴ�ʱ��
	{"local_umask", &tunable_local_umask}, // ����
	{"upload_max_rate", &tunable_upload_max_rate}, // ����ϴ��ٶ�
	{"download_mas_rate", &tunable_download_mas_rate}, // ��������ٶ�
	{NULL, NULL} // �ÿ�ָ��������Ϊ������־
};


//�ַ�������������
static struct parseconf_str_setting
{
	const char *p_setting_name;
	const char **p_variable; // ָ���ַ����Ķ���ָ��
}
parseconf_str_array[] = 
{
	{"listen_address", &tunable_listen_address}, // ip��ַ
	{NULL, NULL}
};


void parseconf_load_file(const char *path) // ���������ļ������������ļ��ж�ȡ��Ϣ
{
	FILE *fp = fopen(path, "r"); // ��ֻ����Ȩ�޴������ļ�
	if(NULL == fp) // �����ʧ��
		ERR_EXIT("parseconf_load_file");

	char setting_line[MAX_SETTING_LINE_SIZE] = {0}; // �����л�����
	while(fgets(setting_line, MAX_SETTING_LINE_SIZE, fp) != NULL) // ѭ����ȡ�����ļ���ÿ�ζ�ȡ�����ļ���һ�У�������ֵΪNULLʱ��˵�������Ѿ���ȡ��ϣ�
	{
		if(setting_line[0]=='\0' || setting_line[0]=='#') // �����ȡ��Ϊ���л�ע���У���������һ��
			continue;
		str_trim_crlf(setting_line); // �ַ������У��е��س��ͻ���

		//����������
		parseconf_load_setting(setting_line);

		memset(setting_line, 0, MAX_SETTING_LINE_SIZE); // ��ʼ�������л�������Ϊ��һ�ζ�ȡ��׼��
	}

	fclose(fp);
}

//listen_port=9100
void parseconf_load_setting(const char *setting) // ��������
{
// �ָ������У�������=������+������ֵ��
	char key[MAX_KEY_SIZE] = {0}; // ��ʼ��������ؼ��ֻ�����
	char value[MAX_VALUE_SIZE] = {0}; // ��ʼ��������ֵ������
	str_split(setting, key, value, '='); // ����"="��������"setting"�ָ��������ؼ���"key"��������ֵ"value"

//��ѯstr������
	const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
	while(p_str_setting->p_setting_name != NULL) // ���������ؼ���δ��������λ�ã��������ѯ
	{
		if(strcmp(key, p_str_setting->p_setting_name) == 0) // ������ҵ��˸����������������������ؼ���key��p_setting_nameƥ��ɹ����򱣴���������ֵ
		{
			const char **p_cur_setting = p_str_setting->p_variable;
			if(*p_cur_setting)
				free((char *)*p_cur_setting);
			*p_cur_setting = strdup(value); // strdup���ڵײ�����malloc���ٿռ䣬��value��ֵ�ŵ��ÿռ��У�Ȼ������ռ佻��p_cur_setting����
			return;
		}
		p_str_setting++;
	}

	//��ѯbool������
	const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
	while(p_bool_setting->p_setting_name != NULL) // ѭ����ѯ
	{
		if(strcmp(key, p_bool_setting->p_setting_name) == 0)
		{
			str_upper(value); // Сд�ַ���ת��Ϊ��д
			int *p_cur_setting = p_bool_setting->p_variable;
			if(strcmp(value, "YES") == 0)
				*p_cur_setting = 1;
			else if(strcmp(value, "NO") == 0)
				*p_cur_setting = 0;
			else
				ERR_EXIT("parseconf_load_setting"); // ����������ֵ����YES��NO����ʾ��������
			return;
		}
		p_bool_setting++;
	}

	//��ѯint������
	const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
	while(p_uint_setting->p_setting_name != NULL)
	{
		if(strcmp(key, p_uint_setting->p_setting_name) == 0)
		{
			unsigned int *p_cur_setting = p_uint_setting->p_variable;
			*p_cur_setting = atoi(value); // atoi���ַ���ת��Ϊ����
			return;
		}
		p_uint_setting++;
	}
}