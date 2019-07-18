//子进程服务

#pragma once

#include <bind.h>

namespace xr_server{
	class child_t
	{
	public:
		struct bind_t*	bind;
	public:
		void run(bind_t* bind, int n_inited_bc);
	};

	extern child_t* g_child;
}
