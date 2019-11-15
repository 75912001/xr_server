//子进程服务

#pragma once

namespace xr_server{
	struct child_t{
		struct bind_t*	bind;
		void run( struct bind_t* bind, int n_inited_bc);
	};

	extern child_t* g_child;
}