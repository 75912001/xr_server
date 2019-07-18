//父进程配置
#pragma once

#include <xr_include.h>

namespace xr_server{
	static const uint8_t PARENT_STATE_STOP = 0;
	static const uint8_t PARENT_STATE_RUN = 1;
	static const uint8_t PARENT_STATE_RESTART = 2;
	typedef int (*ON_PIPE_EVENT)(int fd, epoll_event& r_evs);
	class parent_t
	{
	public:
		parent_t();
		virtual ~parent_t();
		//解析程序传入参数,设置环境,信号处理
		void prase_args(int argc, char** argv);
		//杀死所有子进程(SIGKILL),并等待其退出后返回.
		void killall_children();
		void restart_child_process(struct bind_t* bind, uint32_t bind_idx);
		//处理PIPE管道消息的回调函数
		static int on_pipe_event(int fd, epoll_event& r_evs);
		//0:stop, 1:run, 2:restart
		volatile uint8_t state;
		//子进程ID
		std::atomic_int* child_pids;
		//程序名称
		std::string prog_name;
		//当前目录
		std::string current_dir;
		std::string bench_config_path_suf;
	};
	extern bool g_is_parent;
	extern parent_t* g_parent;
}
