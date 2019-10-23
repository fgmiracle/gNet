
#include "CurrentThread.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h> 
#endif

namespace gNet
{
	namespace CurrentThread
	{
#ifdef _WIN32
		__declspec(thread) ThreadID cachedTid = 0;
#else
		__thread ThreadID cachedTid = 0;
#endif

		ThreadID& tid()
		{
			if (cachedTid == 0)
			{
#ifdef _WIN32
				cachedTid = GetCurrentThreadId();
#else
				cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
#endif
			}

			return cachedTid;
		}

	}
}