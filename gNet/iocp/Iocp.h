#ifndef __GNET_IOCP_H__
#define __GNET_IOCP_H__

#include "gNet/utils/common.h"
#include "gNet/utils/NonCopyable.hpp"
#include "gNet/CurrentThread.h"

namespace gNet {

    class Channel;
    class WakeupChannel;
    class EventLoop : public NonCopyable
    {
    public:
        //using Functor = std::function<void()>;
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
        typedef BOOL(WINAPI *sGetQueuedCompletionStatusEx) (HANDLE, LPOVERLAPPED_ENTRY, ULONG, PULONG, DWORD, BOOL);
        sGetQueuedCompletionStatusEx            mPGetQueuedCompletionStatusEx;
        HANDLE                                  mIOCP;
        CurrentThread::ThreadID                 mSelfThreadID;
        moodycamel::ConcurrentQueue<Functor>    mAsyncProcs;
        std::once_flag                          mOnceInitThreadID;
        std::vector<OVERLAPPED_ENTRY>           mEventEntries;

        std::unique_ptr<WakeupChannel>          mWakeupChannel;
        //const
        static const int                        mInitEventListSize = 16;

// 		std::mutex								mAsyncProcsMutex;
// 		std::vector<Functor>					mAsyncProcs;
// 		std::vector<Functor>					mCopyAsyncProcs;
    };
}
#endif