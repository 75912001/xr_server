#pragma once

#include <xr_tcp.h>
#include "parent.h"

namespace xr_server{
    bool is_parent(){
        return g_is_parent;
    }
    xr::tcp_peer_t* connect(const std::string& ip, uint16_t port);
    int s2peer(xr::tcp_peer_t* peer, const void* data, uint32_t len);
}