#pragma once

#include <xr_tcp.h>
#include "parent.h"

namespace xr_server{
    bool is_parent(){
        return g_is_parent;
    }
    xr::tcp_peer_t* connect(const std::string& ip, uint16_t port);
    int s2peer(xr::tcp_peer_t* peer, const void* data, uint32_t len);

	inline int send_ret(xr::tcp_peer_t* peer, xr_server::proto_head_t& proto_head, xr_server::PROTO_RET ret){
		xr_server::proto_head_t err_out = proto_head;
		err_out.len = xr_server::proto_head_t::PROTO_HEAD_LEN;
		err_out.ret = ret;
		return xr_server::s2peer(peer, (void*)err_out.len, xr_server::proto_head_t::PROTO_HEAD_LEN);
	}	
}