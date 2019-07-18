#include <xr_log.h>
#include <xr_util.h>
#include <xr_xml.h>

#include "bind.h"

namespace {
	const char* s_bind_path =  "./bind.xml";
}//end of namespace

namespace xr_server{
bind_mgr_t* g_bind_mgr;
bind_t::bind_t()
{
	this->id = 0;
	this->port = 0;
	this->restart_cnt = 0;
}

int bind_mgr_t::load()
{
	xr::xml_t xml_parser;
	if (0 != xml_parser.open(s_bind_path)){
		ALERT_LOG("open file [%s]", s_bind_path);
		return FAIL;
	}

	xml_parser.move2children_node();
	xmlNodePtr cur = xml_parser.node_ptr;

	while(cur != NULL){
		//取出节点中的内容
		if (!xmlStrcmp(cur->name, (const xmlChar *)"server")){
			bind_t bind;
			bind.id = xml_parser.get_prop(cur, "id");
			xml_parser.get_prop(cur, "name", bind.name);
			xml_parser.get_prop(cur, "ip", bind.ip);
			bind.port = xml_parser.get_prop(cur, "port");
			xml_parser.get_prop(cur, "data", bind.data);
			INFOR_LOG("server [id:%d, name:%s, ip:%s, port:%d, data:%s]",
				bind.id, bind.name.c_str(),
				bind.ip.c_str(), bind.port, bind.data.c_str());
			this->bind_vec.push_back(bind);
		}
		cur = cur->next;
	}
	return SUCC;
}
}//end namespace xr_server