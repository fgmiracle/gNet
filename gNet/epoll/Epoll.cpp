#include "Epoll.h"
#include "gNet/Channel.h"

namespace gNet {

	class WakeupChannel final : public Channel, public NonCopyable
	{
	public:
		explicit WakeupChannel(gNetfd fd)
			: Channel(nullptr,fd)
		{
		}

		~WakeupChannel()
		{
			Socket::closeSocket(sockFD);
		}

		bool    wakeup()
		{
			uint64_t one = 1;
			return write(sockFD, &one, sizeof(one)) > 0;
			
		}
		void    sendData(const std::string &msg) override { UNUSE(msg);}
		void    sendData(const char *buf, uint32_t inLen) override {UNUSE(buf);UNUSE(inLen);};
		void    connectEstablished() override {}
		void    connectDestroyed() override {}
		void    shutDown() override {}
	private:
		void    doAccept() override {}
		void    doConnect() override {}
		void    canSend() override {}
		void    canRecv() override {
			char temp[1024] = { 0 };
			while (true)
			{
				auto n = read(sockFD, temp, sizeof(temp));
				if (n == -1 || static_cast<size_t>(n) < sizeof(temp))
				{
					break;
				}
			}
		}
		void    doClose(bool bCheck) override { UNUSE(bCheck);}
	};

	EventLoop::EventLoop()
		: mEPOLL(epoll_create(1))
		, mEventEntries(mInitEventListSize)
	{
		gNetfd evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (evtfd <= 0 )
		{
			std::cout << "eventfd err!" << ",err:"<< Socket::socketErr() << std::endl;
		}
		
		mWakeupChannel.reset(new WakeupChannel(evtfd));
		if (!attach(mWakeupChannel.get()))
		{
			std::cout << "attch wakeup fd err!" << std::endl;

			abort();
		}

	}

	EventLoop::~EventLoop()
	{
		close(mEPOLL);
		mEPOLL = -1;
	}

	void EventLoop::loop(int64_t milliseconds)
	{
		tryInitThreadID();

		if (!isInLoopThread())
		{
			std::cout << "thread_id :%d EventLoop is not in initTread" << mSelfThreadID << std::endl;
			return;
		}


		int numComplete = epoll_wait(mEPOLL, mEventEntries.data(), mEventEntries.size(), milliseconds);

		for (int i = 0; i < numComplete; ++i)
		{
			Channel* channel = (Channel*)mEventEntries[i].data.ptr;
			int    event_data = mEventEntries[i].events;

			if (event_data & EPOLLRDHUP)
			{
				channel->canRecv();
				channel->doClose();
				continue;
			}

			if (event_data & EPOLLIN)
			{
				channel->canRecv();
			}

			if (event_data & EPOLLOUT)
			{
				channel->canSend();
			}
		}

		processAsyncProcs();

		//---
		if (static_cast<size_t>(numComplete) == mEventEntries.size())
		{
			mEventEntries.resize(mEventEntries.size() * 2);
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
			while (!mAsyncProcs.enqueue(f)) {
			}
			wakeup();
		}
	}

	bool EventLoop::attach(Channel* ptr)
	{
		struct epoll_event ev = { 0,{ nullptr } };
		ev.events = EPOLLET | EPOLLHUP;
		ev.events = 0;
		ev.data.ptr = (void*)ptr;
		ptr->setEvents(ev.events);
		
		int err = epoll_ctl(mEPOLL, EPOLL_CTL_ADD, ptr->getFd(), &ev);
		if (err != 0)
		{
			std::cout << "ptr->getFd() :" << ptr->getFd() << " attach err:" << err << " socket err:" << Socket::socketErr() <<std::endl;
		}
		
		return err == 0;
	}

	bool EventLoop::detach(Channel* ptr)
	{
		epoll_ctl(mEPOLL, EPOLL_CTL_DEL, ptr->getFd(), nullptr);
		return true;
	}

	void EventLoop::updateChannel(Channel* ptr)
	{
		struct epoll_event ev = { 0,{ nullptr } };
		ev.events = ptr->getEvents();
		ev.data.ptr = ptr;
		epoll_ctl(mEPOLL, EPOLL_CTL_MOD, ptr->getFd(), &ev);
	}

	// private //
	void EventLoop::processAsyncProcs()
	{
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