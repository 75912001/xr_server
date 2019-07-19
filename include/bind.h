//读取bind.xml配置文件
//子进程

#pragma once

#include <xr_include.h>
#include <xr_pipe.h>

namespace xr_server{
	//每个进程的数据
	struct bind_t {
		uint32_t		id;//序号
		std::string		name;//名称
		std::string		ip;
		uint16_t		port;
		std::string		data;
		uint32_t		restart_cnt;//重启过的次数(防止不断的重启)

		xr::pipe_t		send_pipe;//针对子进程的写
		xr::pipe_t		recv_pipe;//针对子进程的读

		bind_t();
	};

	struct bind_mgr_t
	{
		std::vector<bind_t> bind_vec;
		//加载配置文件
		//returns:SUCC:success.负数:error
		int load();
	};
	extern bind_mgr_t* g_bind_mgr;
}