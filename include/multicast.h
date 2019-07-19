//组播

#pragma once

#include <xr_multicast.h>
#include <xr_memory.h>

namespace xr_server{
	#pragma pack(1)
	struct mcast_pkg_header_t {
		uint32_t	cmd;   // for mcast_notify_addr: syn
		mcast_pkg_header_t();
		char* get_body(){
			char* body = (char*)(&this->cmd) + sizeof(this->cmd);
			return body;
		}
	};

	enum E_MCAST_CMD{
		MCAST_CMD_ADDR_SYN	= 1//平时同步用包
	};

	struct mcast_cmd_addr_syn_t {
		uint32_t	svr_id;
		char		name[32];
		char		ip[16];
		uint16_t	port;
		char		data[32];
		mcast_cmd_addr_syn_t();
	};

	#pragma pack()

	class addr_mcast_t : public xr::mcast_t
	{
	public:
		void mcast_notify_addr(E_MCAST_CMD pkg_type = MCAST_CMD_ADDR_SYN);
		void syn();
		void handle_msg(xr::active_buf_t& fd_info);
//		bool get_1st_svr_ip_port(const char* svr_name, std::string& ip, uint16_t& port);
	
		addr_mcast_t();
	private:
		struct addr_mcast_pkg_t {
			char		ip[16];
			uint16_t	port;
			uint32_t syn_time;
			char data[32];
			addr_mcast_pkg_t();
		};

		::time_t syn_sec;
		::time_t next_notify_sec;//下一次通知信息的时间
		typedef std::map<uint32_t, addr_mcast_pkg_t> ADDR_MCAST_SVR_MAP;//KEY:svr_id
		typedef std::map<std::string, ADDR_MCAST_SVR_MAP> ADDR_MCAST_MAP;
		ADDR_MCAST_MAP addr_mcast_map;//地址广播信息
		void add_svr_info(mcast_cmd_addr_syn_t& svr);
	};
	extern addr_mcast_t* g_addr_mcast;
}//end namespace xr_server