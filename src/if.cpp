#include "if.h"
#include <xr_log.h>
#include "tcp.h"
#include "config.h"

namespace xr_server{
bool is_parent(){
    return g_is_parent;
}

int s2peer(xr::tcp_peer_t* peer, const void* data, uint32_t len){
	int send_len = peer->send(data, len);
	if (send_len < 0){
	} else if (send_len < (int)len){
		WARNI_LOG("close socket! send too long [fd:%d, ip:%s, port:%u, len:%u, send_len:%d]",
			peer->fd, xr::net_util_t::ip2str(peer->ip), peer->port, len, send_len);
		peer->send_buf.write((char*)data + send_len, len - send_len);

		uint32_t flag = 0;

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 17)
			flag = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
	#else	
			flag = EPOLLIN | EPOLLOUT;
	#endif

		g_epoll->mod_events(peer->fd, flag);
	}
	return send_len;
}

xr::tcp_peer_t* connect( const std::string& ip, uint16_t port ){
	int fd = xr::tcp_t::connect(ip.c_str(), port, 1, false, 
		g_config->page_size_max,
		g_config->page_size_max);
	if (INVALID_FD == fd){
		return NULL;
	}
	TRACE_LOG("[ip:%s, port:%u, fd:%u]", ip.c_str(), port, fd);
	return g_epoll->add_connect(fd, xr::FD_TYPE::SERVER, ip.c_str(), port);
}

int send_ret(xr::tcp_peer_t* peer, xr_server::proto_head_t& proto_head, xr_server::PROTO_RET ret){
	xr_server::proto_head_t err_out = proto_head;
	err_out.len = xr_server::proto_head_t::PROTO_HEAD_LEN;
	err_out.ret = ret;
	return xr_server::s2peer(peer, (void*)(&err_out.len), xr_server::proto_head_t::PROTO_HEAD_LEN);
}
}