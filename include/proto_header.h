#pragma once

#include <xr_include.h>

namespace xr_server{
typedef uint32_t PROTO_LEN;
typedef uint32_t PROTO_CMD;
typedef uint64_t PROTO_UID;
typedef uint32_t PROTO_SEQ;
typedef uint32_t PROTO_RET;

#pragma pack(1)
//协议包头	
struct proto_head_t
{
    static const uint32_t PROTO_HEAD_LEN = 24;

    PROTO_LEN len;    //报文总长度.长度=包头+包体长度		
	PROTO_CMD cmd;    //命令号	
	PROTO_UID uid;    //玩家id	
    PROTO_SEQ seq;    //序列号，需要原样返回	
    PROTO_RET ret;    //返回值	
    
	proto_head_t (){
		this->clear();
	}
	inline void clear(){
		this->len = 0;
		this->cmd = 0;
		this->uid = 0;
		this->seq = 0;
		this->ret = 0;
	}
	static inline void pack(char* data, PROTO_LEN len,  PROTO_CMD cmd, PROTO_UID uid, PROTO_SEQ seq, PROTO_RET result){
		PROTO_LEN* plen = (PROTO_LEN*)data;
		*plen = len;

		PROTO_CMD* pcmd = (PROTO_CMD*)(plen + sizeof(PROTO_LEN));
		*pcmd = cmd;

		PROTO_UID* puid = (PROTO_UID*)(pcmd + sizeof(PROTO_CMD));
		*puid = uid;

		PROTO_SEQ* pseq = (PROTO_SEQ*)(puid + sizeof(PROTO_UID));
		*pseq = seq;

		PROTO_RET* pret = (PROTO_RET*)(pseq + sizeof(PROTO_SEQ));
		*pret = result;
	}
	inline void unpack(const void* data)
    {
		PROTO_LEN* plen = (PROTO_LEN*)data;
		this->len = *plen;

		PROTO_CMD* pcmd = (PROTO_CMD*)(plen + sizeof(PROTO_LEN));
		this->cmd = *pcmd;

		PROTO_UID* puid = (PROTO_UID*)(pcmd + sizeof(PROTO_CMD));
		this->uid = *puid;

		PROTO_SEQ* pseq = (PROTO_SEQ*)(puid + sizeof(PROTO_UID));
		this->seq = *pseq;

		PROTO_RET* pret = (PROTO_RET*)(pseq + sizeof(PROTO_SEQ));
		this->ret = *pret;
	}
    inline PROTO_LEN get_all_len(){
        return this->len;
    }
    inline PROTO_LEN get_body_len(){
        return this->len - proto_head_t::PROTO_HEAD_LEN;
    }
};

#pragma pack()
}//end of namespace xr_server