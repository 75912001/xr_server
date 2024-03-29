#include <xr_log.h>
#include <xr_util.h>
#include <xr_xml.h>

#include "bind.h"

namespace
{
const char *s_bind_path = "./bind.xml";
} //end of namespace

namespace xr_server
{
bind_mgr_t *g_bind_mgr;
bind_t::bind_t()
{
	this->id = 0;
	this->port = 0;
	this->restart_cnt = 0;
}

int bind_mgr_t::load()
{
	xr::xml_t xml;
	if (0 != xml.open(s_bind_path))
	{
		BOOT_LOG(FAIL, "open file [%s]", s_bind_path);
		return FAIL;
	}

	xml.move2children_node();
	xmlNodePtr cur = xml.node_ptr;

	while (cur != NULL)
	{
		//取出节点中的内容
		if (!xmlStrcmp(cur->name, (const xmlChar *)"server"))
		{
			bind_t bind;
			bind.id = xml.get_prop(cur, "id");
			xml.get_prop(cur, "name", bind.name);
			xml.get_prop(cur, "ip", bind.ip);
			bind.port = xml.get_prop(cur, "port");
			xml.get_prop(cur, "data", bind.data);
			this->bind_vec.push_back(bind);
		}
		cur = cur->next;
	}
	return SUCC;
}
} //end namespace xr_server