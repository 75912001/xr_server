#include "dll.h"
#include <xr_log.h>

namespace
{
#define DLFUNC(v, name)                                           \
	{                                                             \
		v = (decltype(v))dlsym(this->handle, name);               \
		if (NULL != (error = dlerror()))                          \
		{                                                         \
			ALERT_LOG("DLSYM ERROR [error:%s, fn:%p]", error, v); \
			dlclose(this->handle);                                \
			this->handle = NULL;                                  \
			goto out;                                             \
		}                                                         \
	}
} //end of namespace

namespace xr_server
{
dll_t *g_dll;
int dll_t::register_plugin(const char *liblogic_path)
{
	char *error;
	int ret_code = FAIL;

	this->handle = dlopen(liblogic_path, RTLD_NOW);
	if ((error = dlerror()) != NULL)
	{
		BOOT_LOG(ret_code, "DLOPEN ERROR [error:%s,%s]", error, liblogic_path);
		return ret_code;
	}

	DLFUNC(this->on_tcp_srv.on_events, "on_events");
	DLFUNC(this->on_tcp_srv.on_cli_conn, "on_cli_conn");
	DLFUNC(this->on_tcp_srv.on_cli_pkg, "on_cli_pkg");
	DLFUNC(this->on_tcp_srv.on_srv_pkg, "on_srv_pkg");
	DLFUNC(this->on_tcp_srv.on_cli_conn_closed, "on_cli_conn_closed");
	DLFUNC(this->on_tcp_srv.on_svr_conn_closed, "on_svr_conn_closed");
	DLFUNC(this->on_tcp_srv.on_get_pkg_len, "on_get_pkg_len");
	DLFUNC(this->on_tcp_srv.on_mcast_pkg, "on_mcast_pkg");
	DLFUNC(this->on_tcp_srv.on_addr_mcast_pkg, "on_addr_mcast_pkg");
	DLFUNC(this->on_tcp_srv.on_udp_pkg, "on_udp_pkg");
	DLFUNC(this->on_tcp_srv.on_init, "on_init");
	DLFUNC(this->on_tcp_srv.on_fini, "on_fini");

	ret_code = SUCC;
out:
	BOOT_LOG(ret_code, "dlopen [file name:%s, state:%s]",
			 liblogic_path, (0 != ret_code ? "FAIL" : "OK"));
	return ret_code;
}

dll_t::dll_t()
{
	this->handle = NULL;
}

dll_t::~dll_t()
{
	if (NULL != this->handle)
	{
		dlclose(this->handle);
		this->handle = NULL;
	}
}
} //end namespace xr_server