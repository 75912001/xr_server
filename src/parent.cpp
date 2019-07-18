#include "parent.h"
#include <xr_log.h>
#include "config.h"
#include "bind.h"
#include "tcp.h"
#include "child.h"



namespace xr_server{
	bool g_is_parent = true;
	parent_t* g_parent;
}

namespace {
	 //重命名core文件(core.进程id)
	void rename_core(int pid)
	{
		::chmod("core", 777);
		::chown("core", 0, 0);
		char core_name[1024] ={0};
		::sprintf(core_name, "core.%d", pid);
		::rename("core", core_name);
	}

	std::vector<std::string> g_argvs;

	void sigterm_handler(int signo) 
	{
		//停止服务（不重启）
		CRITI_LOG("SIG_TERM FROM [stop:true, restart:false]");
		xr_server::g_parent->state = xr_server::PARENT_STATE_STOP;
	}

	void sighup_handler(int signo) 
	{
		//停止&&重启服务
		CRITI_LOG("SIGHUP FROM [restart:true, stop:true]");

		xr_server::g_parent->state = xr_server::PARENT_STATE_RESTART;
	}

	//为了在core之前输出所有的日志
	void sigsegv_handler(int signo) 
	{
		CRITI_LOG("SIGSEGV FROM");

 		signal(SIGSEGV, SIG_DFL);
 		kill(getpid(), signo);
	}

#if 0
	//man 2 sigaction to see si->si_code
	void sigchld_handler(int signo, siginfo_t *si, void * p) 
	{
		CRIT_LOG("sigchld from [pid=%d, uid=%d", si->si_pid, si->si_uid);
		switch (si->si_code) {
		case CLD_DUMPED:
			::chmod("core", 700);
			::chown("core", 0, 0);
			char core_name[1024] ={0};
			::sprintf(core_name, "core.%d", si->si_pid);
			::rename("core", core_name);
			break;
		}
	}
#endif

	void set_signal()
	{
		//服务端关闭已连接客户端，客户端接着发数据产生问题，
		//1. 当服务器close一个连接时，若client端接着发数据。根据TCP协议的规定，
		//会收到一个RST响应，client再往这个服务器发送数据时，系统会发出一个SIGPIPE信号给进程，
		//告诉进程这个连接已经断开了，不要再写了。根据信号的默认处理规则SIGPIPE信号的
		//默认执行动作是terminate(终止、退出),所以client会退出。若不想客户端退出可以把SIGPIPE设为SIG_IGN
		//如:    signal(SIGPIPE,SIG_IGN);
		//这时SIGPIPE交给了系统处理。
		//2. 客户端write一个已经被服务器端关闭的sock后，返回的错误信息Broken pipe.
		//1）broken pipe的字面意思是“管道破裂”。broken pipe的原因是该管道的读端被关闭。
		//2）broken pipe经常发生socket关闭之后（或者其他的描述符关闭之后）的write操作中
		//3）发生broken pipe错误时，进程收到SIGPIPE信号，默认动作是进程终止。
		//4）broken pipe最直接的意思是：写入端出现的时候，另一端却休息或退出了，
		//因此造成没有及时取走管道中的数据，从而系统异常退出
		signal(SIGPIPE, SIG_IGN);

		struct sigaction sa;
		::memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigterm_handler;
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGQUIT, &sa, NULL);

#if 0
		sa.sa_flags = SA_RESTART|SA_SIGINFO;
		sa.sa_sigaction = sigchld_handler;
		sigaction(SIGCHLD, &sa, NULL);
#endif

		sa.sa_handler = sighup_handler;
		sigaction(SIGHUP, &sa, NULL);

		sa.sa_handler = sigsegv_handler;
		sigaction(SIGSEGV, &sa, NULL);

		sigset_t sset;
		sigemptyset(&sset);
		sigaddset(&sset, SIGSEGV);
		sigaddset(&sset, SIGBUS);
		sigaddset(&sset, SIGABRT);
		sigaddset(&sset, SIGILL);
		sigaddset(&sset, SIGCHLD);
		sigaddset(&sset, SIGFPE);
		sigprocmask(SIG_UNBLOCK, &sset, &sset);
	}

}//end of namespace

namespace xr_server{
parent_t::parent_t()
{
	this->state = PARENT_STATE_RUN;
	this->child_pids = NULL;
}

void parent_t::prase_args( int argc, char** argv )
{
 	this->prog_name = argv[0];
	char* dir = ::get_current_dir_name();
	this->current_dir = dir;
	free(dir);

	//setrlimit
	{
		struct rlimit rlim;

		/* set open files */
		rlim.rlim_cur = g_config->max_fd_num;
		rlim.rlim_max = g_config->max_fd_num;
		if (0 != setrlimit(RLIMIT_NOFILE, &rlim)) {
			CRITI_LOG("INIT FD RESOURCE FAILED [OPEN FILES NUMBER:%d]", g_config->max_fd_num);
		}

		/* set core dump */
		rlim.rlim_cur = g_config->core_size;
		rlim.rlim_max = g_config->core_size;
		if (0 != setrlimit(RLIMIT_CORE, &rlim)) {
			CRITI_LOG("INIT CORE FILE RESOURCE FAILED [CORE DUMP SIZE:%d]",
				g_config->core_size);
		}
	}
	set_signal();
	//save_argv
	{
		for (int i = 0; i < argc; i++){
			std::string str = argv[i];
			g_argvs.push_back(str);
		}
	}
	
	daemon(1, 1);
}

parent_t::~parent_t()
{
	if (2 == this->state && !this->prog_name.empty()) {
		this->killall_children();

		ALERT_LOG("SERVER RESTARTING...");
		::chdir(this->current_dir.c_str());
		char* argvs[200];
		int i = 0;
		FOREACH(g_argvs, it){
			argvs[i] = const_cast<char*>(it->c_str());
			i++;
		}

		execv(this->prog_name.c_str(), argvs);
		ALERT_LOG("RESTART FAILED...");
	}
	SAFE_DELETE_ARR(this->child_pids);
}

void parent_t::killall_children()
{
	for (uint32_t i = 0; i < g_bind_mgr->bind_vec.size(); ++i) {
		if (0 != this->child_pids[i]) {
			kill(this->child_pids[i], SIGTERM/*SIGKILL*/);
		}
	}
}

void parent_t::restart_child_process( bind_t* bind, uint32_t bind_idx)
{
	if (PARENT_STATE_STOP == this->state){
		//在关闭服务器时,防止子进程先收到信号退出,父进程再次创建子进程.
		return;
	}
	bind->recv_pipe.close(xr::E_PIPE_INDEX_WRONLY);
	bind->send_pipe.close(xr::E_PIPE_INDEX_RDONLY);

	if (0 != bind->recv_pipe.create()
		|| 0 != bind->send_pipe.create()){
		ALERT_LOG("pipe create err");
		return;
	}

	pid_t pid;

	if ( (pid = fork ()) < 0 ) {
		CRITI_LOG("fork failed: %s", strerror(errno));
	} else if (pid > 0) {
		//parent
		g_bind_mgr->bind_vec[bind_idx].recv_pipe.close(xr::E_PIPE_INDEX_RDONLY);
		g_bind_mgr->bind_vec[bind_idx].send_pipe.close(xr::E_PIPE_INDEX_WRONLY);
		g_epoll->add_connect(bind->send_pipe.read_fd(), xr::FD_TYPE_PIPE, NULL, 0);
		this->child_pids[bind_idx] = pid;
	} else { 
		//child
		g_child->run(&g_bind_mgr->bind_vec[bind_idx], g_bind_mgr->bind_vec.size());
	}
}

int parent_t::on_pipe_event( int fd, epoll_event& r_evs )
{
	if (r_evs.events & EPOLLHUP) {
		if (g_is_parent) {
			pid_t pid;
			int	status;
			while ((pid = waitpid (-1, &status, WNOHANG)) > 0) {
				for (uint32_t i = 0; i < g_bind_mgr->bind_vec.size(); ++i) {
					if (g_parent->child_pids[i] == pid) {
						g_parent->child_pids[i] = 0;
						bind_t& bind = g_bind_mgr->bind_vec[i];
						CRITI_LOG("child process crashed![olid:%u, olname:%s, fd:%d, restart_cnt;%u, restart_cnt_max:%u, pid=%d]",
							bind.id, bind.name.c_str(), fd, bind.restart_cnt, g_config->restart_cnt_max, pid);
						rename_core(pid);
						// prevent child process from being restarted again and again forever
						if (++bind.restart_cnt <= g_config->restart_cnt_max) {
							g_parent->restart_child_process(&bind, i);
						}else{
							//关闭pipe(不使用PIPE,并且epoll_wait不再监控PIPE)
							bind.recv_pipe.close(xr::E_PIPE_INDEX_WRONLY);
							bind.send_pipe.close(xr::E_PIPE_INDEX_RDONLY);
						}
						break;
					}
				}
			}
		} else {
			// Parent Crashed
			CRITI_LOG("parent process crashed!");
			g_parent->state = PARENT_STATE_STOP;
			return 0;
		}
	} else {
		CRITI_LOG("unuse ???[]");
	}
	return 0;
}
}//end namespace xr_server