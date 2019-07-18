#include "child.h"
#include "parent.h"
#include <xr_log.h>
#include "config.h"
#include <xr_timer.h>
#include "tcp.h"
#include "dll.h"
#include "bind.h"

namespace xr_server{
child_t* g_child;
void child_t::run( bind_t* bind, int n_inited_bc )
{
	g_bind_mgr->bind = bind;
	g_is_parent = false;
	SAFE_DELETE(xr::g_log);
	{
		xr::g_log = new xr::log_t;
		std::string pre_name = xr::T2string(bind->id) + "_";
		int ret = xr::g_log->init(g_config->log_dir.c_str(), (xr::E_LOG_LEVEL)g_config->log_level, pre_name.c_str(), g_config->log_save_next_file_interval_min);
		if (SUCC != ret){
			std::cout << "??? log init" << std::endl;
			return;
		}
	}
	SAFE_DELETE(xr::g_timer);
	{
		xr::g_timer = new xr::timer_t;
	}
	SAFE_DELETE(g_epoll);
	{
		g_epoll = new xr_server::epoll_t;
		if (SUCC != g_epoll->create()){
			std::cout << "??? epoll_t::create" << std::endl;
			return;
		}
		g_epoll->on_pipe_event = xr_server::parent_t::on_pipe_event;
	}

	for (int i = 0; i != n_inited_bc; ++i ) {
		g_bind_mgr->bind[i].recv_pipe.close(xr::E_PIPE_INDEX_WRONLY);
		g_bind_mgr->bind[i].send_pipe.close(xr::E_PIPE_INDEX_RDONLY);
	}

	this->bind = bind;
	char prefix[10] = { 0 };
	int  len = snprintf(prefix, 8, "%u", this->bind->id);
	prefix[len] = '_';

	//初始化子进程
	int ret = 0;
	if (0 != (ret = g_epoll->create())){
		ALERT_LOG("g_epoll->create err [ret:%d]", ret);
		return;
	}

	//g_epoll->on_pipe_event = dll_t::on_pipe_event;
	g_epoll->add_connect(this->bind->recv_pipe.read_fd(), xr::FD_TYPE_PIPE, NULL, 0);

	//g_epoll->epoll_wait_time_out = EPOLL_TIME_OUT;
	if (0 != g_epoll->listen(this->bind->ip.c_str(),
		this->bind->port, g_config->listen_num, g_config->page_size_max)){
		xr::g_log->boot(ERR, 0, "server listen err [ip:%s, port:%u]", this->bind->ip.c_str(), this->bind->port);
		return;
	}

#ifndef EL_ASYNC_USE_THREAD
	if ( SUCC != g_dll->on_tcp_srv.on_init()) {
		ALERT_LOG("FAIL TO INIT WORKER PROCESS. [id=%u, name=%s]", this->bind->id, this->bind->name.c_str());
		goto fail;
	}
#endif
/* 
	//创建组播
	if (!g_config->mcast_ip.empty()){
		if (SUCC != g_mcast.create(g_config->mcast_ip, 
			g_config->mcast_port, g_config->mcast_incoming_if,
			g_config->mcast_outgoing_if)){
				ALERT_LOG("mcast.create err[ip:%s]", g_config->mcast_ip.c_str());
			return;
		}else{
			ser->add_connect(g_mcast.fd,
				xr::FD_TYPE_MCAST, g_config->mcast_ip.c_str(), g_config->mcast_port);
		}
	}

	//创建地址信息组播
	{
		g_addr_mcast = new xr_server::addr_mcast_t;
	}
	if (!g_config->addr_mcast_ip.empty()){
		if (SUCC != g_addr_mcast->create(g_config->addr_mcast_ip,
			g_config->addr_mcast_port, g_config->addr_mcast_incoming_if,
			g_config->addr_mcast_outgoing_if)){
				ALERT_LOG("addr mcast.create err [ip:%s]", g_config->addr_mcast_ip.c_str());
				return;
		} else {
			ser->add_connect(g_addr_mcast->fd,
				xr::FD_TYPE_ADDR_MCAST, g_config->addr_mcast_ip.c_str(), g_config->addr_mcast_port);
			//取消第一次发包的标记		
			//g_addr_mcast->mcast_notify_addr(MCAST_CMD_ADDR_1ST);
			g_addr_mcast->mcast_notify_addr();
		}
	}
*/
#ifdef EL_ASYNC_USE_THREAD	
	g_service_logic = new el_async::service_logic_thread_t;
	g_service_logic->start(NULL);
#endif

	// 这里网络线程和逻辑线程退出先后会影响到数据是否能发送出去
	// 先干掉监听端口,再设置所有的FD不接收数据,处理剩余的逻辑数据,关闭逻辑线程,关闭网络线程[11/15/2013 MengLingChao]
	g_epoll->run();
#ifdef EL_ASYNC_USE_THREAD
	SAFE_DELETE(g_service_logic);
#endif
#ifndef EL_ASYNC_USE_THREAD	
fail:
#endif
	SAFE_DELETE(g_epoll);
	SAFE_DELETE(xr::g_log);
	SAFE_DELETE(xr::g_timer);

	exit(0);
}
}