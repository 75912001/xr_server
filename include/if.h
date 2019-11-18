#pragma once

#include <xr_tcp.h>
#include "parent.h"
#include "proto_header.h"

namespace xr_server
{
bool is_parent();
xr::tcp_peer_t *connect(const std::string &ip, uint16_t port);
int s2peer(xr::tcp_peer_t *peer, const void *data, uint32_t len);
int send_ret(xr::tcp_peer_t *peer, xr_server::proto_head_t &proto_head, xr_server::PROTO_RET ret);
} // namespace xr_server