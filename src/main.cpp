
#include <xr_util.h>
#include <xr_log.h>
#include <xr_timer.h>
#include "parent.h"

#include "config.h"
#include "tcp.h"
#include "bind.h"
#include "dll.h"
#include "child.h"

int main(int argc, char* argv[])
{
	int ret = SUCC;
	xr_server::g_is_parent = true;

	std::vector<std::string> argvs;
	{	
		for (int i = 0; i < argc; i++){
			std::string str = argv[i];
			argvs.push_back(str);
     	   std::cout << str << std::endl;
		}
		if (argvs.size() < 2){
       	 std::cout <<"??? agrv num" << std::endl;
       	 return FAIL;
		}
	}
	{
    	xr_server::g_config = new xr_server::config_t;
		if (SUCC != xr_server::g_config->load(argvs[1].c_str())){
			std::cout << "??? config.ini load" << std::endl;
			return FAIL;
		}
	}
	{
		xr::g_log = new xr::log_t;
		int ret = xr::g_log->init(xr_server::g_config->log_dir.c_str(), 
			(xr::E_LOG_LEVEL)xr_server::g_config->log_level, NULL,
			xr_server::g_config->log_save_next_file_interval_min);
		if (SUCC != ret){
			std::cout << "??? log init" << std::endl;
			return FAIL;
		}
	}
	{
		xr::g_timer = new xr::timer_t;
	}
	{
		xr_server::g_epoll = new xr_server::epoll_t;
		if (SUCC != xr_server::g_epoll->create()){
			std::cout << "??? epoll_t::create" << std::endl;
			return FAIL;
		}
		xr_server::g_epoll->on_pipe_event = xr_server::parent_t::on_pipe_event;
	}
	{
		xr_server::g_bind_mgr = new xr_server::bind_mgr_t;
		if (SUCC != xr_server::g_bind_mgr->load()){
			std::cout << "??? bind.xml load" << std::endl;
			return FAIL;
		}
	}
	{
		xr_server::g_parent = new xr_server::parent_t;
		xr_server::g_parent->child_pids = new std::atomic_int[xr_server::g_bind_mgr->bind_vec.size()];

		xr_server::g_parent->prase_args(argc, argv);
	}
	{
		xr_server::g_dll = new xr_server::dll_t;
		if (SUCC != xr_server::g_dll->register_plugin(xr_server::g_config->liblogic_path.c_str())){
			std::cout << "??? dll register_plugin" << std::endl;
			return FAIL;
		}
	}
	{
		if (SUCC != xr_server::g_dll->on_tcp_srv.on_init()) {
			std::cout << "??? FAILED TO INIT PARENT PROCESS" << std::endl;
			return FAIL;
		}
	}
	{
		::open("/dev/null", O_RDONLY);
		::open("/dev/null", O_WRONLY);
		::open("/dev/null", O_RDWR);
	}
	{
		for (uint32_t i = 0; i != xr_server::g_bind_mgr->bind_vec.size(); ++i ) {
			xr_server::bind_t& bind = xr_server::g_bind_mgr->bind_vec[i];

			if (0 != bind.recv_pipe.create()
				|| 0 != bind.send_pipe.create()){
				std::cout << "??? pipe create err" << std::endl;
				return FAIL;
			}
			pid_t pid;
			if ( (pid = fork()) < 0 ) {
				std::cout << "??? fork child process err id:" << bind.id << std::endl;
				return FAIL;
			} else if (pid > 0) {//父进程
				xr_server::g_bind_mgr->bind_vec[i].recv_pipe.close(xr::E_PIPE_INDEX_RDONLY);
				xr_server::g_bind_mgr->bind_vec[i].send_pipe.close(xr::E_PIPE_INDEX_WRONLY);
				xr_server::g_epoll->add_connect(bind.send_pipe.read_fd(), xr::FD_TYPE_PIPE, NULL, 0);
				xr_server::g_parent->child_pids[i] = pid;

				xr_server::g_epoll->run();
			} else {
				//子进程
				xr_server::g_child->run(&bind, i + 1);
				return SUCC;
			}
		}
	}
	xr_server::g_parent->killall_children();
	{
		SAFE_DELETE(xr_server::g_config);
		SAFE_DELETE(xr::g_log);
		SAFE_DELETE(xr::g_timer);
		SAFE_DELETE(xr_server::g_bind_mgr);
		SAFE_DELETE(xr_server::g_dll);
		SAFE_DELETE(xr_server::g_parent);
		SAFE_DELETE(xr_server::g_epoll);
	}
	return ret;
}
