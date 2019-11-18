//读取config.ini配置文件

#pragma once

#include <xr_file_ini.h>

namespace xr_server
{

struct config_t
{
	config_t();
	std::string liblogic_path; //代码段so路径

	uint32_t max_fd_num;	//打开fd的最大数量.默认20000
	uint32_t page_size_max; //数据包的最大字节数.默认81920
	uint32_t listen_num;	//tcp listen 数量.默认1024

	std::string log_dir;					  //日志目录
	uint32_t log_level;						  //日志等级
	uint32_t log_save_next_file_interval_min; //每多少时间(分钟)重新保存文件中(新文件)

	uint32_t core_size;		  //core文件的大小，字节.默认2147483648U
	uint32_t restart_cnt_max; //最大重启次数.默认100

	std::string mcast_in_if;  //组播 接收//默认为0
	std::string mcast_out_if; //组播 发送//默认为0
	std::string mcast_ip;	 //239.X.X.X组播地址//默认为0
	uint32_t mcast_port;	  //239.X.X.X组播端口//默认为0

	std::string addr_mcast_in_if;  //地址信息 组播 接收//默认为0
	std::string addr_mcast_out_if; //地址信息 组播 发送//默认为0
	std::string addr_mcast_ip;	 //239.X.X.X地址信息 组播地址//默认为0
	uint32_t addr_mcast_port;	  //239.X.X.X地址信息 组播端口//默认为0

	//so调用获取配置项config.ini中的数据(自行配置)
	//获取配置项数据
	std::string get_val_str(const char *section, const char *name);
	bool get_val_uint32(const char *section, const char *name, uint32_t &val);
	uint32_t get_val_uint32_def(const char *section, const char *name, uint32_t def = 0);
	//加载配置文件config.ini
	//returns:0:正确.其它:错误
	int load(const char *path_suf);

private:
	xr::file_ini_t file_ini;
};
extern config_t *g_config;
} // namespace xr_server
