#ifndef __GNET_EPOLL_H__
#define __GNET_EPOLL_H__

#include "gNet/utils/common.h"
#include "gNet/utils/NonCopyable.hpp"
#include "gNet/CurrentThread.h"

namespace gNet {

	class Channel;
	class WakeupChannel;
	class EventLoop : public NonCopyable
	{
	public:
		typedef std::function<void()> Functor;
	
	public:
		EventLoop();
		~EventLoop();
		void	loop(int64_t milliseconds);
		bool    wakeup();
		void    pushAsyncProc(Functor &&f);
		void    pushAftercProc(Functor &&f);

		bool    attach(Channel* ptr);
		bool    detach(Channel* ptr);
		void    updateChannel(Channel* ptr);

		inline bool isInLoopThread() const
		{
			return mSelfThreadID == CurrentThread::tid();
		}

		void assertInLoopThread()
		{
			if (!isInLoopThread())
			{
				std::cout << "EventLoop::abortNotInLoopThread - EventLoop " << this
					<< " was created in threadId_ = " << mSelfThreadID
					<< ", current thread id = " << CurrentThread::tid() << std::endl;
			}
		}

	private:
		void    processAsyncProcs();
		void    tryInitThreadID();
	private:
		gNetfd									mEPOLL;
		std::unique_ptr<WakeupChannel>          mWakeupChannel;
		static const int                        mInitEventListSize = 16;
		CurrentThread::ThreadID                 mSelfThreadID;
		std::once_flag                          mOnceInitThreadID;
		moodycamel::ConcurrentQueue<Functor>    mAsyncProcs;
		std::vector<epoll_event>				mEventEntries;
	};
}
#endif
