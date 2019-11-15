//tcp server epoll 
//socket:socket(2)调用从2.6.27开始支持SOCK_NONBLOCK参数了	
//accept4:从Linux2.6.28开始的accept4支持SOCK_NOBLOCK参数,accept出来的fd就已经是非阻塞的了
//这样就可以减少一次fcntl(2)的调用开销了
#pragma  once

#include <xr_tcp_server.h>
#include "parent.h"

namespace xr_server{
	class epoll_t : public xr::tcp_srv_t{
	public:
		epoll_t();
		virtual int create();

		//Create a listening socket fd
		//ip the binding ip address. If this argument is assigned with 0,
		//	then INADDR_ANY will be used as the binding address
		//port the binding port.
		//listen_num the maximum length to which the queue of pending connections for sockfd may grow
		//bufsize maximum socket send and receive buffer in bytes, should be less than 10 * 1024 * 1024
		//return the newly created listening fd on success, -1 on error
		virtual int listen(const char* ip, uint16_t port, uint32_t listen_num, int bufsize);
		virtual int run();
		virtual xr::tcp_peer_t* add_connect(int fd, 
			xr::FD_TYPE fd_type, const char* ip, uint16_t port);
		int mod_events(int fd, uint32_t flag);
		int add_events(int fd, uint32_t flag);
		//关闭连接
		//do_calback true:调用客户端注册的回调函数.通知被关闭, false:不做通知
		void close_peer(xr::tcp_peer_t& tcp_peer, bool do_calback = true);
	private:
		void handle_peer_msg(xr::tcp_peer_t& tcp_peer);
		void handle_listen();
		int handle_send(xr::tcp_peer_t& tcp_peer);
	};
	extern epoll_t* g_epoll;
}//end namespace xr_server