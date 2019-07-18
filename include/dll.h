//接口

#pragma once

#include <xr_tcp_server.h>

namespace xr_server{
	class dll_t
	{
	public:
		void* handle;
        dll_t();
		virtual ~dll_t();
		xr::on_tcp_srv_t on_tcp_srv;
		//注册插件
		int register_plugin(const char* liblogic_path);
	};
	extern dll_t* g_dll;
}//end namespace xr_server



