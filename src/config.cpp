#include "config.h"

namespace xr_server{
config_t* g_config;
int config_t::load(const char* config_path)
{
	this->file_ini.load(config_path);

	if (0 != this->file_ini.get("plugin", "liblogic", this->liblogic_path)){
        std::cout << "??? plugin,liblogic" << std::endl;
		return FAIL;
	}
	this->file_ini.get("tcp", "max_fd_num", this->max_fd_num);
	this->file_ini.get("tcp", "page_size_max", this->page_size_max);
	this->file_ini.get("tcp", "listen_num", this->listen_num);

	if (0 != this->file_ini.get("log", "path", this->log_dir)){
        std::cout << "??? log,path" << std::endl;
		return FAIL;
	}
	this->file_ini.get("log", "level", this->log_level);
	this->file_ini.get("log", "save_next_file_interval_min", this->log_save_next_file_interval_min);

	this->file_ini.get("core", "size", this->core_size);
	this->file_ini.get("core", "restart_cnt_max", this->restart_cnt_max);

	this->file_ini.get("multicast", "in_if", this->mcast_in_if);
	this->file_ini.get("multicast", "out_if", this->mcast_out_if);
	this->file_ini.get("multicast", "ip", this->mcast_ip);
	this->file_ini.get("multicast", "port", this->mcast_port);

	this->file_ini.get("addr_multicast", "in_if", this->addr_mcast_in_if);
	this->file_ini.get("addr_multicast", "out_if", this->addr_mcast_out_if);
	this->file_ini.get("addr_multicast", "ip", this->addr_mcast_ip);
	this->file_ini.get("addr_multicast", "port", this->addr_mcast_port);

	return SUCC;
}

config_t::config_t()
{
	this->max_fd_num = 20000;
	this->page_size_max = 81920;
    this->listen_num = 1024;

	this->log_level = 8;
	this->log_save_next_file_interval_min = 0;
	
	this->core_size = 2147483648U;
	this->restart_cnt_max = 100;

	this->mcast_port = 0;
	this->addr_mcast_port = 0;	
}

std::string config_t::get_val_str(const char* section, const char* name)
{
	std::string str;

	this->file_ini.get(section, name, str);

	return str;
}

bool config_t::get_val_uint32( const char* section, const char* name, uint32_t& val )
{
	if (SUCC != this->file_ini.get(section, name, val)){
		return false;
	}
	return true;
}

uint32_t config_t::get_val_uint32_def(const char* section, const char* name, uint32_t def)
{
	return this->file_ini.get_def(section, name, def);
}
}//end namespace xr_server