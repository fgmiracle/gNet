#include "Iocp.h"
#include "gNet/Channel.h"

namespace gNet {
    class WakeupChannel final : public Channel, public NonCopyable
    {
    public:
        explicit WakeupChannel(HANDLE iocp) 
            : mIOCP(iocp)
        {
        }

        void    wakeup()
        {
            PostQueuedCompletionStatus(mIOCP, 0, (ULONG_PTR)this, &mWakeupOvl.mOverLapped);
        }
		void    sendData(const std::string &msg) override {}
		void    sendData(const char *buf, uint32_t inLen) override {};
		void    connectEstablished() override {}
		void    connectDestroyed() override {}
		void    shutDown() override {}
    private:
		void    doAccept(OverLapped *overLapped) override {}
		void    doConnect() override {}
		void    canSend() override {}
		void    canRecv() override {}
        void    doClose(bool bCheck) override {}

    private:
        HANDLE          mIOCP;
		OverLappedWakeup  mWakeupOvl;
    };

    /*
        Eventloop impl
    */
    EventLoop::EventLoop()
        : mIOCP(CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1))
        , mSelfThreadID(0)
        , mEventEntries(mInitEventListSize)
    {
       mWakeupChannel.reset(new WakeupChannel(mIOCP));

       mPGetQueuedCompletionStatusEx = NULL;
        auto kernel32_module = GetModuleHandleA("kernel32.dll");
        if (kernel32_module != NULL) 
        {
            mPGetQueuedCompletionStatusEx = (sGetQueuedCompletionStatusEx)GetProcAddress(
                kernel32_module,
                "GetQueuedCompletionStatusEx");
            FreeLibrary(kernel32_module);
        }
        
    }

    EventLoop::~EventLoop()
    {
		CloseHandle(mIOCP);
		mIOCP = INVALID_HANDLE_VALUE;
    }

    ////
	void EventLoop::loop(int64_t milliseconds)
	{
		tryInitThreadID();

		if (!isInLoopThread())
		{
			std::cout << "thread_id :%d EventLoop is not in initTread" << mSelfThreadID << std::endl;
			return;
		}
		
		ULONG numComplete = 0;
		if (mPGetQueuedCompletionStatusEx != nullptr)
		{
			if (!mPGetQueuedCompletionStatusEx(mIOCP,
				 &*mEventEntries.begin(),
				static_cast<ULONG>(mEventEntries.size()),
				&numComplete,
				static_cast<DWORD>(milliseconds),
				false))
			{
				numComplete = 0;
			}
		}
		else
		{
			do
			{
				GetQueuedCompletionStatus(mIOCP,
					&mEventEntries[numComplete].dwNumberOfBytesTransferred,
					&mEventEntries[numComplete].lpCompletionKey,
					&mEventEntries[numComplete].lpOverlapped,
					(numComplete == 0) ? static_cast<DWORD>(milliseconds) : 0);
			} while (mEventEntries[numComplete].lpOverlapped != nullptr && 
				mEventEntries[numComplete].dwNumberOfBytesTransferred > 0 && 
				++numComplete < mEventEntries.size());
		}

		for (ULONG i = 0; i < numComplete; ++i)
		{
			Channel* channel = (Channel*)mEventEntries[i].lpCompletionKey;

			if (channel == nullptr)
			{
				continue;
			}

			if (mEventEntries[i].lpOverlapped != nullptr)
			{
				OverLapped * pSocketLapped = CONTAINING_RECORD(mEventEntries[i].lpOverlapped, OverLapped, mOverLapped);
				//const auto pSocketLapped = reinterpret_cast<const OverLapped*>(mEventEntries[i].lpOverlapped);

				/*
				if (channel && 0 == mEventEntries[i].dwNumberOfBytesTransferred && (pSocketLapped->GetOperationType() == enOperationType_Send ||
					pSocketLapped->GetOperationType() == enOperationType_Recv))
				{

					channel->doClose();
					continue;
				}
				*/
				if (pSocketLapped->GetOperationType() == enOperationType::enOperationType_Send)
				{
					channel->canSend();
				}
				else if (pSocketLapped->GetOperationType() == enOperationType::enOperationType_Recv)
				{
					channel->canRecv();
				}
				else if (pSocketLapped->GetOperationType() == enOperationType::enOperationType_Accpet)
				{
					channel->doAccept(pSocketLapped);
				}
				else if (pSocketLapped->GetOperationType() == enOperationType::enOperationType_Connect)
				{
					channel->doConnect();
				}
				else if (pSocketLapped->GetOperationType() == enOperationType::enOperationType_Wakeup)
				{
					;
				}
				else
				{
					std::cout << "GetQueuedCompletionStatus---> Unkown type" <<pSocketLapped->GetOperationType() << std::endl;
					std::cout.flush();
					//assert(false);
				}
			}

		}

		processAsyncProcs();

		//---
		if (static_cast<size_t>(numComplete) == mEventEntries.size())
		{
			mEventEntries.resize(mEventEntries.size()*2);
		}
    }

    bool EventLoop::wakeup()
    {
        if (!isInLoopThread())
        {
            mWakeupChannel->wakeup();
            return true;
        }

        return false;
    }

	void EventLoop::pushAftercProc(Functor &&f)
	{

		while (!mAsyncProcs.enqueue(f)) {
		}

		if (!isInLoopThread())
		{
			wakeup();
		}
	}

    void EventLoop::pushAsyncProc(Functor &&f)
    {
        if (isInLoopThread())
        {
            f();
        }
        else
        {
// 			{
// 				std::lock_guard<std::mutex> lck(mAsyncProcsMutex);
// 				mAsyncProcs.emplace_back(std::move(f));
// 			}
            while (!mAsyncProcs.enqueue(f)) {
            }
            wakeup();
        }
    }
    
    bool EventLoop::attach(Channel* ptr)
    {
        return CreateIoCompletionPort((HANDLE)ptr->getFd(), mIOCP, (ULONG_PTR)ptr, 0) != nullptr;
    }

    bool EventLoop::detach(Channel* ptr)
    {
		return true;
    }

    void EventLoop::updateChannel(Channel* ptr)
    {
    }

    // private
    void EventLoop::processAsyncProcs()
    {
// 		{
// 			std::lock_guard<std::mutex> lck(mAsyncProcsMutex);
// 			mCopyAsyncProcs.swap(mAsyncProcs);
// 		}
// 
// 		for (const auto& x : mCopyAsyncProcs)
// 		{
// 			x();
// 		}
// 		mCopyAsyncProcs.clear();
        Functor f;
        while (mAsyncProcs.try_dequeue(f)) {
            f();
        }
    }

    inline void EventLoop::tryInitThreadID()
    {
        std::call_once(mOnceInitThreadID, [this]() {
            mSelfThreadID = CurrentThread::tid();
			std::cout << "start loop --> mSelfThreadID:" << mSelfThreadID << std::endl << std::flush;
        });		
    }
}