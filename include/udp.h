//tcp server epoll 
//socket:socket(2)调用从2.6.27开始支持SOCK_NONBLOCK参数了	
//accept4:从Linux2.6.28开始的accept4支持SOCK_NOBLOCK参数,accept出来的fd就已经是非阻塞的了
//这样就可以减少一次fcntl(2)的调用开销了
#pragma  once

#include <xr_tcp_server.h>

namespace xr_server{
	struct udp_t{
		static void handle_peer_mcast_msg(xr::tcp_peer_t& tcp_peer);
		static void handle_peer_add_mcast_msg(xr::tcp_peer_t& tcp_peer);
		static void handle_peer_udp_msg(xr::tcp_peer_t& tcp_peer);
	};
}//end namespace xr_server