#ifndef __GNET_CURRENTTHREAD_H__
#define __GNET_CURRENTTHREAD_H__

#include "utils/common.h"

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace gNet 
{
	namespace CurrentThread
	{
#ifdef _WIN32
		typedef DWORD ThreadID;
		extern __declspec(thread) ThreadID cachedTid;
#else
		typedef int32_t ThreadID;
		extern __thread ThreadID cachedTid;
#endif

		ThreadID& tid();
	}
}

#endif // !__CURRENTTHREAD_H__

