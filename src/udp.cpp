#include "udp.h"
#include "dll.h"
#include "multicast.h"

namespace
{
const uint32_t RECV_BUF_LEN = 1024 * 1024; //1024K
char recv_buf_tmp[RECV_BUF_LEN] = {0};
//接收对方消息
//return	int 0:断开 >0:接收的数据长度
int recv_peer_msg(xr::tcp_peer_t &tcp_peer)
{
	int len = 0;

	int sum_len = 0;
	while (1)
	{
		len = tcp_peer.recv(recv_buf_tmp, sizeof(recv_buf_tmp));
		if (likely(len > 0))
		{
			if (len < (int)sizeof(recv_buf_tmp))
			{
				tcp_peer.recv_buf.write(recv_buf_tmp, len);
				return len;
			}
			else if (sizeof(recv_buf_tmp) == len)
			{
				tcp_peer.recv_buf.write(recv_buf_tmp, len);
				sum_len += len;
				continue;
			}
		}
		else if (0 == len)
		{
			return 0;
		}
		else if (len == -1)
		{
			return sum_len;
		}
	}
	return sum_len;
}

//接收对方消息
//return	int 接收的数据长度
int recv_peer_udp_msg(xr::tcp_peer_t &tcp_peer, sockaddr_in &peer_addr)
{
	socklen_t from_len = sizeof(sockaddr_in);
	int len = HANDLE_EINTR(::recvfrom(tcp_peer.fd, recv_buf_tmp, sizeof(recv_buf_tmp), 0,
									  (sockaddr *)&(peer_addr), &from_len));
	if (0 < len)
	{
		tcp_peer.recv_buf.write(recv_buf_tmp, len);
	}
	return len;
}
} //end namespace

namespace xr_server
{

void udp_t::handle_peer_mcast_msg(xr::tcp_peer_t &tcp_peer)
{
	recv_peer_msg(tcp_peer);
	if (tcp_peer.recv_buf.write_pos > 0)
	{
		g_dll->on_tcp_srv.on_mcast_pkg(tcp_peer.recv_buf.data, tcp_peer.recv_buf.write_pos);
		tcp_peer.recv_buf.pop(tcp_peer.recv_buf.write_pos);
	}
}

void udp_t::handle_peer_add_mcast_msg(xr::tcp_peer_t &tcp_peer)
{
	recv_peer_msg(tcp_peer);
	if (tcp_peer.recv_buf.write_pos > 0)
	{
		g_addr_mcast->handle_msg(tcp_peer.recv_buf); //todo  注意线程安全 [11/14/2013 MengLingChao]
		tcp_peer.recv_buf.pop(tcp_peer.recv_buf.write_pos);
	}
}

void udp_t::handle_peer_udp_msg(xr::tcp_peer_t &tcp_peer)
{
	sockaddr_in peer_addr;
	::memset(&(peer_addr), 0, sizeof(peer_addr));
	recv_peer_udp_msg(tcp_peer, peer_addr);
	if (tcp_peer.recv_buf.write_pos > 0)
	{
		g_dll->on_tcp_srv.on_udp_pkg(tcp_peer.fd, tcp_peer.recv_buf.data,
									 tcp_peer.recv_buf.write_pos, &peer_addr, sizeof(peer_addr));
		tcp_peer.recv_buf.pop(tcp_peer.recv_buf.write_pos);
	}
}
} //end namespace xr_server