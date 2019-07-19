#include "multicast.h"
#include "bind.h"
#include <xr_random.h>
#include <xr_timer.h>
#include "dll.h"
#include <xr_log.h>


	const uint32_t ADDR_MCAST_SYN_TIME_OUT_SEC = 60;//同步地址超时秒数
}//end namespace 

namespace xr_server{
addr_mcast_t* g_addr_mcast;
mcast_pkg_header_t::mcast_pkg_header_t()
{
	this->cmd = 0;
}

void addr_mcast_t::mcast_notify_addr( E_MCAST_CMD pkg_type )
{
	mcast_cmd_addr_syn_t pkg;
	mcast_pkg_header_t hdr;
	char* data = new char[sizeof(hdr) + sizeof(pkg)];
	hdr.cmd = pkg_type;
	pkg.svr_id     = g_child->bind->id;
	::strncpy(pkg.name, g_child->bind->name.c_str(), std::min(sizeof(pkg.name), g_child->bind->name.size());
	::strncpy(pkg.ip, g_child->bind->ip.c_str(), std::min(sizeof(pkg.ip), g_child->bind->ip.size()));
	pkg.port = g_child->bind->port;
	::strncpy(pkg.data, g_child->bind->data.c_str(), std::min(sizeof(pkg.data), g_child->bind->data.size()));
	memcpy(data, &hdr, sizeof(hdr));
	memcpy(data + sizeof(hdr), &(pkg), sizeof(pkg));
	this->send(data, sizeof(hdr) + sizeof(pkg));
	SAFE_DELETE_ARR(data);
	this->next_notify_sec = xr::random_t::random(15, 25) + (xr::g_timer)->now_sec();
}

void addr_mcast_t::syn()
{
	if (this->syn_sec < xr::g_timer->now_sec()){
		//清理过期的地址同步信息
		auto it = this->addr_mcast_map.begin();
		for (; this->addr_mcast_map.end() != it;){
			std::string svr_name = it->first;
			ADDR_MCAST_SVR_MAP& info = it->second;
			for (auto it2 = info.begin(); info.end() != it2;){
				uint32_t svr_id = it2->first;
				addr_mcast_pkg_t& syn = it2->second;
				if ((xr::g_timer->now_sec() - syn.syn_time) > ADDR_MCAST_SYN_TIME_OUT_SEC){
					g_dll->on_tcp_srv.on_addr_mcast_pkg(svr_id, svr_name.c_str(),
						syn.ip, syn.port, syn.data, 0);
					info.erase(it2++);
				} else {
					it2++;
				}
			}
			if (info.empty()){
				this->addr_mcast_map.erase(it++);
			} else {
				it++;
			}
		}
		this->syn_sec = xr::g_timer->now_sec();
	}

	//定时广播自己的地址信息
	if (this->next_notify_sec < (int32_t)xr::g_timer->now_sec()){
		this->mcast_notify_addr();
	}
}

addr_mcast_t::addr_mcast_t()
{
	this->syn_sec = 0;
	this->next_notify_sec = 0;
	::srand(::getpid());
}

void addr_mcast_t::handle_msg(xr::active_buf_t& recv_buf)
{
	mcast_pkg_header_t* hdr = (mcast_pkg_header_t*)recv_buf.data;
	mcast_cmd_addr_syn_t* add_mcast_info = (mcast_cmd_addr_syn_t*)hdr->get_body();
	std::string strname = add_mcast_info->name;
	if (strname == g_child->bind->name
		&& add_mcast_info->svr_id == g_child->bind->id){
		return;
	}
	auto it = this->addr_mcast_map.find(strname);
	switch (hdr->cmd)
	{
	case MCAST_CMD_ADDR_SYN:
		{
			if (this->addr_mcast_map.end() != it
				&& it->second.end() != it->second.find(add_mcast_info->svr_id)){
				addr_mcast_pkg_t& syn_info = it->second[add_mcast_info->svr_id];
				syn_info.syn_time = xr::g_timer->now_sec();
			} else {
				this->add_svr_info(*add_mcast_info);
				this->mcast_notify_addr();
			}
			g_dll->on_tcp_srv.on_addr_mcast_pkg(add_mcast_info->svr_id, 
				add_mcast_info->name, add_mcast_info->ip, add_mcast_info->port, add_mcast_info->data, 1);
		}
		break;
	default:
		ERROR_LOG("cmd:%#x, ip:%s, name:%s, port:%u, id:%u", hdr->cmd,
			add_mcast_info->ip, add_mcast_info->name, add_mcast_info->port, add_mcast_info->svr_id);
		break;
	}
}

void addr_mcast_t::add_svr_info( mcast_cmd_addr_syn_t& svr )
{
	addr_mcast_pkg_t amp;
	strcpy(amp.ip, svr.ip);
	amp.port = svr.port;
	::memcpy(amp.data, svr.data, sizeof(svr.data));
	this->addr_mcast_map[svr.name][svr.svr_id] = amp;
}

// bool addr_mcast_t::get_1st_svr_ip_port( const char* svr_name, std::string& ip, uint16_t& port )
// {
// 	FOREACH(this->addr_mcast_map, it){
// 		if (it->first == svr_name){
// 			ADDR_MCAST_SVR_MAP& am = it->second;
// 			FOREACH(am, it2){
// 				addr_mcast_pkg_t& ampt = it2->second;
// 				ip = ampt.ip;
// 				port = ampt.port;
// 				return true;
// 			}
// 		}
// 	}
// 	return false;
// }
mcast_cmd_addr_syn_t::mcast_cmd_addr_syn_t()
{
	::memset(this, 0, sizeof(*this));
}

addr_mcast_t::addr_mcast_pkg_t::addr_mcast_pkg_t()
{
	::memset(this->ip, 0, sizeof(this->ip));
	this->syn_time = xr::g_timer->now_sec();
}
}