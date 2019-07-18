
#include <xr_util.h>
#include <xr_log.h>
#include "tcp.h"
#include "dll.h"
#include <xr_timer.h>
#include "multicast.h"
#include "config.h"
#include <xr_ecode.h>





namespace {
	const uint32_t RECV_BUF_LEN = 1024*1024*10;//1024K*10
	char recv_buf_tmp[RECV_BUF_LEN] = {0};
	//接收对方消息
	//return	int 0:断开 >0:接收的数据长度
	int recv_peer_msg(xr::tcp_peer_t& tcp_peer)
	{
		int len = 0;

		int sum_len = 0;
		while (1) {
			len = tcp_peer.recv(recv_buf_tmp, sizeof(recv_buf_tmp));
			if (likely(len > 0)){
				if (len < (int)sizeof(recv_buf_tmp)){
					tcp_peer.recv_buf.write(recv_buf_tmp, len);
					return len;
				}else if (sizeof(recv_buf_tmp) == len){
					tcp_peer.recv_buf.write(recv_buf_tmp, len);
					sum_len += len;
					continue;
				}
			} else if (0 == len){
				return 0;
			} else if (len == -1) {
				return sum_len;
			}
		}
		return sum_len;
	}

	//接收对方消息
	//return	int 接收的数据长度
	int recv_peer_udp_msg(xr::tcp_peer_t& tcp_peer, sockaddr_in& peer_addr)
	{
		socklen_t from_len = sizeof(sockaddr_in);
		int len = HANDLE_EINTR(::recvfrom(tcp_peer.fd, recv_buf_tmp, sizeof(recv_buf_tmp), 0,
			(sockaddr*)&(peer_addr), &from_len));
		tcp_peer.recv_buf.write(recv_buf_tmp, len);
		return len;
	}

}//end namespace 

namespace xr_server{
epoll_t* g_epoll;
int epoll_t::run()
{
	int event_num = 0;//事件的数量
	epoll_event evs[this->cli_fd_value_max];
#ifndef EL_ASYNC_USE_THREAD
	while(likely(PARENT_STATE_RUN == g_parent->state) || g_dll->on_tcp_srv.on_fini()){
#else
	while(likely(PARENT_STATE_RUN == g_parent->state)){
#endif//EL_ASYNC_USE_THREAD
		event_num = HANDLE_EINTR(::epoll_wait(this->fd, evs, this->cli_fd_value_max, 40));
#ifndef EL_ASYNC_USE_THREAD
		xr::g_timer->renew_time();
		if (!g_is_parent){
			g_addr_mcast->syn_info();
		}
		g_dll->on_tcp_srv.on_events();
#else
		g_service_logic->notify_event();
#endif//EL_ASYNC_USE_THREAD
		if (0 == event_num){//time out
			continue;
		}else if (unlikely(-1 == event_num)){
			ALERT_LOG("epoll wait err [%s]", ::strerror(errno));
			return ERR;
		}
		//handling event
		for (int i = 0; i < event_num; ++i){
			xr::tcp_peer_t& fd_info = this->tcp_peer[evs[i].data.fd];
			uint32_t events = evs[i].events;
			if ( unlikely(xr::FD_TYPE_PIPE == fd_info.fd_type) ) {
				if (0 == this->on_pipe_event(fd_info.fd, evs[i])) {
					continue;
				} else {
					return ERR;
				}
			}
			//可读或可写(可同时发生,并不互斥)
			if(EPOLLOUT & events){//该套接字可写
				if (this->handle_send(fd_info) < 0){
					CRITI_LOG("EPOLLOUT fd:%d", fd_info.fd);
					this->close_peer(fd_info);
					continue;
				}
			}
			if(EPOLLIN & events){//接收并处理其他套接字的数据
				switch (fd_info.fd_type)
				{
				case xr::FD_TYPE_LISTEN:
					this->handle_listen();
					continue;
					break;
				case xr::FD_TYPE_CLI:
				case xr::FD_TYPE_SVR:
					this->handle_peer_msg(fd_info);
					break;
				case xr::FD_TYPE_MCAST:
					//this->handle_peer_mcast_msg(fd_info);
					break;
				case xr::FD_TYPE_ADDR_MCAST:
					//this->handle_peer_add_mcast_msg(fd_info);
					break;
				case xr::FD_TYPE_UDP:
					//this->handle_peer_udp_msg(fd_info);
					break;
				default:
					break;
				}
			}
			if (EPOLLHUP & events){
				// EPOLLHUP: When close of a fd is detected (ie, after receiving a RST segment:
				//				   the client has closed the socket, and the server has performed
				//				   one write on the closed socket.)
				this->close_peer(fd_info);

				// After receiving EPOLLRDHUP or EPOLLHUP, we can still read data from the fd until
				// read() returns 0 indicating EOF is reached. So, we should alway call read on receving
				// this kind of events to aquire the remaining data and/or EOF. (Linux-2.6.18)
				// todo 可读 [10/17/2013 MengLingChao]
				continue;				
			}
			if(EPOLLERR & events){
				//只有采取动作时,才能知道是否对方异常.即对方突然断掉,是不可能
				//有此事件发生的.只有自己采取动作(当然自己此刻也不知道),read,
				//write时，出EPOLLERR错，说明对方已经异常断开
				//EPOLLERR 是服务器这边出错(自己出错当然能检测到,对方出错你咋能
				//直到啊)
				CRITI_LOG("EPOLLERR fd:%d", fd_info.fd);
				this->close_peer(fd_info);
				continue;
			}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
			if(EPOLLRDHUP & events){
				//EPOLLRDHUP (since Linux 2.6.17)
				//Stream socket peer closed connection, or shut down writing
				//half of connection.  (This flag is especially useful for
				//writing simple code to detect peer shutdown when using Edge
				//Triggered monitoring.)
				//EPOLLRDHUP: If a client send some data to a server and than close the connection
				//immediately, the server will receive RDHUP and IN at the same time.
				//Under ET mode, this can make a sockfd into CLOSE_WAIT state.
				//对端close
				//EPOLLRDHUP = 0x2000,
				//#define EPOLLRDHUP EPOLLRDHUP
				CRITI_LOG("EPOLLRDHUP fd:%d", fd_info.fd);
				this->close_peer(fd_info);
				// After receiving EPOLLRDHUP or EPOLLHUP, we can still read data from the fd until
				// read() returns 0 indicating EOF is reached. So, we should alway call read on receving
				// this kind of events to aquire the remaining data and/or EOF. (Linux-2.6.18)
				// todo 可读 [10/17/2013 MengLingChao]
				continue;
			}
#endif
			if(!(events & EPOLLOUT) && !(events & EPOLLIN)){
				ERROR_LOG("events:%u", events);
			}
		}
	}

	return 0;
}

epoll_t::epoll_t()
{
	this->cli_fd_value_max = g_config->max_fd_num;
	this->on_pipe_event = NULL;
}

int epoll_t::listen(const char* ip,
	uint16_t port, uint32_t listen_num, int bufsize)
{

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
		this->listen_fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	#else
		this->listen_fd = ::socket(PF_INET, SOCK_STREAM, 0);
	#endif

	if (INVALID_FD == this->listen_fd){
		ALERT_LOG("create socket err [%s]", ::strerror(errno));
		return FAIL;
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
		xr::file_t::set_io_block(this->listen_fd, false);
	#endif
	int ret = xr::net_util_t::set_reuse_addr(this->listen_fd);
	if (SUCC != ret){
		xr::file_t::close_fd(this->listen_fd);
		return FAIL;
	}
	ret = xr::net_util_t::set_recvbuf(this->listen_fd, bufsize);
	if (SUCC != ret){
		xr::file_t::close_fd(this->listen_fd);
		return FAIL;
	}
	ret = xr::net_util_t::set_sendbuf(this->listen_fd, bufsize);
	if(SUCC != ret){
		xr::file_t::close_fd(this->listen_fd);
		return FAIL;
	}

	if (SUCC != this->bind(ip, port)){
		xr::file_t::close_fd(this->listen_fd);
		ALERT_LOG("bind socket err [%s]", ::strerror(errno));
		return FAIL;
	}

	if (0 != ::listen(this->listen_fd, listen_num)){
		xr::file_t::close_fd(this->listen_fd);
		ALERT_LOG("listen err [%s]", ::strerror(errno));
		return FAIL;
	}

	if (NULL == this->add_connect(this->listen_fd, xr::FD_TYPE_LISTEN, ip, port)){
		xr::file_t::close_fd(this->listen_fd);
		ALERT_LOG("add connect err[%s]", ::strerror(errno));
		return FAIL;
	}

	this->ip = xr::net_util_t::ip2int(ip);
	this->port = port;
	return 0;
}

int epoll_t::create()
{
	if ((this->fd = ::epoll_create(this->cli_fd_value_max)) < 0) {
		ALERT_LOG("EPOLL_CREATE FAILED [ERROR:%s]", strerror (errno));
		return FAIL;
	}

	this->tcp_peer = (xr::tcp_peer_t*) new xr::tcp_peer_t[this->cli_fd_value_max];
	if (NULL == this->tcp_peer){
		xr::file_t::close_fd(this->fd);
		ALERT_LOG ("CALLOC CLI_FD_INFO_T FAILED [MAXEVENTS=%d]", this->cli_fd_value_max);
		return FAIL;
	}
	return SUCC;
}

int epoll_t::add_events( int fd, uint32_t flag )
{
	epoll_event ev;
	ev.events = flag;
	ev.data.fd = fd;

	int ret = HANDLE_EINTR(::epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &ev));
	if (0 != ret){
		CRITI_LOG ("epoll_ctl add fd:%d error:%s", fd, strerror(errno));
		return FAIL;
	}

	return SUCC; 
}

xr::tcp_peer_t* epoll_t::add_connect( int fd,
	xr::E_FD_TYPE fd_type, const char* ip, uint16_t port )
{
	uint32_t flag;

	switch (fd_type) {
	default:
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 17)
			flag = EPOLLIN | EPOLLRDHUP;
	#else
			flag = EPOLLIN;
	#endif
		break;
	}

	if (0 != this->add_events(fd, flag)) {
		CRITI_LOG("add events err [fd:%d, flag:%u]", fd, flag);
		return NULL;
	}

	xr::tcp_peer_t& cfi = this->tcp_peer[fd];
	cfi.update(fd, fd_type, ip, port);

	INFOR_LOG("fd:%d, fd type:%d, ip:%s, port:%u", 
		fd, fd_type, xr::net_util_t::ip2str(cfi.ip), cfi.port);
	return &cfi;
}

void epoll_t::handle_peer_msg( xr::tcp_peer_t& tcp_peer )
{
	int ret = recv_peer_msg(tcp_peer);
	if (ret > 0){
#ifdef EL_ASYNC_USE_THREAD
		g_service_logic->add_handle_peer_msg(&tcp_peer);
#else
		int available_len = 0;
		while (0 != (available_len = g_dll->on_tcp_srv.on_get_pkg_len(&tcp_peer,
			tcp_peer.recv_buf.data, tcp_peer.recv_buf.write_pos))){	
			if (-1 == available_len){
				CRITI_LOG("close socket! available_len [fd:%d, ip:%s, port:%u]",
					tcp_peer.fd, xr::net_util_t::ip2str(tcp_peer.ip), tcp_peer.port);
				this->close_peer(tcp_peer);
				break;
			} else if (available_len > 0 && (int)tcp_peer.recv_buf.write_pos >= available_len){
				if (xr::FD_TYPE_CLI == tcp_peer.fd_type){
					if ((int)xr::ECODE_SYS::DISCONNECT_PEER == g_dll->on_tcp_srv.on_cli_pkg(&tcp_peer, tcp_peer.recv_buf.data, available_len)){
						WARNI_LOG("close socket! ret [fd:%d, ip:%s, port:%u]",
							tcp_peer.fd, xr::net_util_t::ip2str(tcp_peer.ip), tcp_peer.port);
						this->close_peer(tcp_peer);
						break;
					}
				} else if (xr::FD_TYPE_SVR == tcp_peer.fd_type){
					g_dll->on_tcp_srv.on_srv_pkg(&tcp_peer, tcp_peer.recv_buf.data, available_len);
				}
				tcp_peer.recv_buf.pop(available_len);
			}else{
				break;
			}
			if (0 == tcp_peer.recv_buf.write_pos){
				break;
			}
		}
	}else if (0 == ret || -1 == ret){
		INFOR_LOG("close socket by peer [fd:%d, ip:%s, port:%u, ret:%d]", 
			tcp_peer.fd, xr::net_util_t::ip2str(tcp_peer.ip), tcp_peer.port, ret);
		this->close_peer(tcp_peer);
	}
#endif
}

void epoll_t::handle_listen()
{
	sockaddr_in peer;
	::memset(&peer, 0, sizeof(peer));
	int peer_fd = this->accept(peer, g_config->page_size_max,
		g_config->page_size_max);
	if (unlikely(peer_fd < 0) || unlikely(peer_fd >= this->cli_fd_value_max)){
		ALERT_LOG("accept err [%s] fd:%d", ::strerror(errno), peer_fd);
		if (peer_fd > 0){
			xr::file_t::close_fd(peer_fd);
		}
	}else{
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
			xr::file_t::set_io_block(peer_fd, false);
		#endif
		INFOR_LOG("client accept [ip:%s, port:%u, new_socket:%d]",
			inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), peer_fd);
		xr::tcp_peer_t* tcp_peer = this->add_connect(peer_fd, xr::FD_TYPE_CLI, xr::net_util_t::ip2str(peer.sin_addr.s_addr), ntohs(peer.sin_port));
		if (NULL != tcp_peer){
			g_dll->on_tcp_srv.on_cli_conn(tcp_peer);
		} else {
			xr::file_t::close_fd(peer_fd);
		}
	}
}

int epoll_t::handle_send( xr::tcp_peer_t& tcp_peer )
{
#ifdef EL_ASYNC_USE_THREAD
	tcp_peer.lock_mutex.lock();
#endif
	int send_len = tcp_peer.send(tcp_peer.send_buf.data, tcp_peer.send_buf.write_pos);
	if (send_len > 0){
		if (0 == tcp_peer.send_buf.pop(send_len)){
			uint32_t flag;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 17)

//EPOLLRDHUP (since Linux 2.6.17)
//              Stream socket peer closed connection, or shut down writing half of connection.  (This flag is especially
//              useful for writing simple code to detect peer shutdown when using Edge Triggered monitoring.)
			flag = EPOLLIN | EPOLLRDHUP;
#else
			flag = EPOLLIN;
#endif
			this->mod_events(tcp_peer.fd, flag);
		}
	}
#ifdef EL_ASYNC_USE_THREAD
	tcp_peer.lock_mutex.ulock();
#endif
	return send_len;
}

int epoll_t::mod_events( int fd, uint32_t flag )
{
	epoll_event ev;
	ev.events = flag;
	ev.data.fd = fd;

	int ret = HANDLE_EINTR(::epoll_ctl(this->fd, EPOLL_CTL_MOD, fd, &ev));
	if (0 != ret){
		CRITI_LOG ("epoll_ctl mod fd:%d error:%s", fd, strerror(errno));
		return FAIL;
	}
	return SUCC; 
}

void epoll_t::close_peer( xr::tcp_peer_t& tcp_peer, bool do_calback /*= true*/ )
{
#ifdef EL_ASYNC_USE_THREAD
	tcp_peer.lock_mutex.lock();
#endif
	
	if (xr::FD_TYPE_CLI == tcp_peer.fd_type){
		INFOR_LOG("close socket cli [fd:%d, ip:%s, port:%u]", tcp_peer.fd, 
			xr::net_util_t::ip2str(tcp_peer.ip), tcp_peer.port);
		if (do_calback){
			g_dll->on_tcp_srv.on_cli_conn_closed(tcp_peer.fd);
		}
	} else if (xr::FD_TYPE_SVR == tcp_peer.fd_type){
		INFOR_LOG("close socket svr [fd:%d, ip:%s, port:%u]", tcp_peer.fd, 
			xr::net_util_t::ip2str(tcp_peer.ip), tcp_peer.port);
		if (do_calback){
			g_dll->on_tcp_srv.on_svr_conn_closed(tcp_peer.fd);
		}
	}

	epoll_event ev;
	int ret = HANDLE_EINTR(::epoll_ctl(this->fd, EPOLL_CTL_DEL, tcp_peer.fd, &ev));
	if (0 != ret){
		CRITI_LOG ("epoll_ctl del fd:%d error:%s", tcp_peer.fd, strerror(errno));
	}

	tcp_peer.close();
#ifdef EL_ASYNC_USE_THREAD
	tcp_peer.lock_mutex.ulock();
#endif
}

}//end namespace xr_server