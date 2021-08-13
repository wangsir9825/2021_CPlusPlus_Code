#include"parseconf.h"
#include"tunable.h"
#include"str.h"


//bool类型配置项结构体类型
static struct parseconf_bool_setting 
{
	const char *p_setting_name; //配置项的名字
	int        *p_variable;     //配置项的值（用一个整型指针变量指向这个值）
}
parseconf_bool_array[] = // 定义一个bool类型配置项结构体类型数组
{
	{"pasv_enable", &tunable_pasv_enable}, // 是否开启被动模式
	{"port_enable", &tunable_port_enable}, // 是否开启主动模式
	{NULL, NULL} // 用空指针类型作为结束标志
};

//int类型配置项结构体类型
static struct parseconf_uint_setting
{
	const char   *p_setting_name;
	unsigned int *p_variable;
}
parseconf_uint_array[] = 
{
	{"listen_port", &tunable_listen_port}, // 端口
	{"max_clients", &tunable_max_clients}, // 最大连接数
	{"max_per_ip" , &tunable_max_per_ip}, // 每ip最大连接数
	{"accept_timeout", &tunable_accept_timeout}, // accept超时时间
	{"connect_timeout", &tunable_connect_timeout}, // connect超时时间
	{"idle_session_timeout", &tunable_idle_session_timeout}, // 控制连接超时等待时间
	{"data_connection_timeout", &tunable_data_connection_timeout}, // 数据传输超时等待时间
	{"local_umask", &tunable_local_umask}, // 掩码
	{"upload_max_rate", &tunable_upload_max_rate}, // 最大上传速度
	{"download_mas_rate", &tunable_download_mas_rate}, // 最大下载速度
	{NULL, NULL} // 用空指针类型作为结束标志
};


//字符串类型配置项
static struct parseconf_str_setting
{
	const char *p_setting_name;
	const char **p_variable; // 指向字符串的二级指针
}
parseconf_str_array[] = 
{
	{"listen_address", &tunable_listen_address}, // ip地址
	{NULL, NULL}
};


void parseconf_load_file(const char *path) // 加载配置文件，即从配置文件中读取信息
{
	FILE *fp = fopen(path, "r"); // 以只读的权限打开配置文件
	if(NULL == fp) // 如果打开失败
		ERR_EXIT("parseconf_load_file");

	char setting_line[MAX_SETTING_LINE_SIZE] = {0}; // 配置行缓存区
	while(fgets(setting_line, MAX_SETTING_LINE_SIZE, fp) != NULL) // 循环读取配置文件，每次读取配置文件的一行（当返回值为NULL时，说明内容已经读取完毕）
	{
		if(setting_line[0]=='\0' || setting_line[0]=='#') // 如果读取的为空行或注释行，则跳过这一行
			continue;
		str_trim_crlf(setting_line); // 字符串剪切，切掉回车和换行

		//解析配置行
		parseconf_load_setting(setting_line);

		memset(setting_line, 0, MAX_SETTING_LINE_SIZE); // 初始化配置行缓存区，为下一次读取做准备
	}

	fclose(fp);
}

//listen_port=9100
void parseconf_load_setting(const char *setting) // 解析配置
{
// 分割配置行（配置行=配置项+配置项值）
	char key[MAX_KEY_SIZE] = {0}; // 初始化配置项关键字缓存区
	char value[MAX_VALUE_SIZE] = {0}; // 初始化配置项值缓存区
	str_split(setting, key, value, '='); // 根据"="把配置行"setting"分割成配置项关键字"key"和配置项值"value"

//查询str配置项
	const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
	while(p_str_setting->p_setting_name != NULL) // 如果配置项关键字未到达最后的位置，则继续查询
	{
		if(strcmp(key, p_str_setting->p_setting_name) == 0) // 如果查找到了该配置项，即解析出的配置项关键字key与p_setting_name匹配成功，则保存该配置项的值
		{
			const char **p_cur_setting = p_str_setting->p_variable;
			if(*p_cur_setting)
				free((char *)*p_cur_setting);
			*p_cur_setting = strdup(value); // strdup会在底层利用malloc开辟空间，将value的值放到该空间中，然后将这个空间交给p_cur_setting管理
			return;
		}
		p_str_setting++;
	}

	//查询bool配置项
	const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
	while(p_bool_setting->p_setting_name != NULL) // 循环查询
	{
		if(strcmp(key, p_bool_setting->p_setting_name) == 0)
		{
			str_upper(value); // 小写字符串转化为大写
			int *p_cur_setting = p_bool_setting->p_variable;
			if(strcmp(value, "YES") == 0)
				*p_cur_setting = 1;
			else if(strcmp(value, "NO") == 0)
				*p_cur_setting = 0;
			else
				ERR_EXIT("parseconf_load_setting"); // 如果配置项的值不是YES或NO则提示解析出错
			return;
		}
		p_bool_setting++;
	}

	//查询int配置项
	const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
	while(p_uint_setting->p_setting_name != NULL)
	{
		if(strcmp(key, p_uint_setting->p_setting_name) == 0)
		{
			unsigned int *p_cur_setting = p_uint_setting->p_variable;
			*p_cur_setting = atoi(value); // atoi将字符串转换为数字
			return;
		}
		p_uint_setting++;
	}
}