#ifndef __GNET_SESSIONID_H__
#define __GNET_SESSIONID_H__

#include "utils/common.h"

namespace gNet {
	class Session
	{
	public:
		Session() 
			: mSessionID(0){}
		
		uint64_t getSessionID()
		{
			return ++mSessionID;
		}
	private:
		std::atomic<uint64_t>            mSessionID;
	};


#define GETSESSIONID() \
	Singleton<Session>::getInstance().getSessionID()
	
}

#endif
