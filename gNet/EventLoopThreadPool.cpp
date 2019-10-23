#include "EventLoopThreadPool.h"

#ifdef _WIN32
#include "iocp/Iocp.h"
#else
#include <epoll/Epoll.h>
#endif

namespace gNet {
    class IOLoopData : public NonCopyable, public std::enable_shared_from_this<IOLoopData>
    {
    public:
        typedef std::shared_ptr<IOLoopData> PTR;
        static    PTR    Create(std::shared_ptr<EventLoop> eventloop, std::shared_ptr<std::thread> iothread);
        const std::shared_ptr<EventLoop>&    getEventLoop() const;

        inline void  wakeupLoop()
        {
            mEventLoop->wakeup();
        }

        inline void  joinableThread()
        {
            if (mIOThread->joinable())
            {
                mIOThread->join();
            }
        }

    private:
        explicit IOLoopData(std::shared_ptr<EventLoop> eventloop, std::shared_ptr<std::thread> iothread);

    private:
        const std::shared_ptr<EventLoop>mEventLoop;
        std::shared_ptr<std::thread>    mIOThread;

        friend class EventLoopThreadPool;
    };

	IOLoopData::PTR IOLoopData::Create(std::shared_ptr<EventLoop> eventloop, std::shared_ptr<std::thread> iothread)
	{
		struct make_shared_enabler : public IOLoopData
		{
			make_shared_enabler(std::shared_ptr<EventLoop> eventloop, std::shared_ptr<std::thread> iothread) :
				IOLoopData(std::move(eventloop), std::move(iothread))
			{

			}
		};

		return std::make_shared<make_shared_enabler>(std::move(eventloop), std::move(iothread));
	}

	IOLoopData::IOLoopData(std::shared_ptr<EventLoop> eventloop, std::shared_ptr<std::thread> iothread)
		: mEventLoop(std::move(eventloop))
		, mIOThread(std::move(iothread))
	{

	}

	const std::shared_ptr<EventLoop>& IOLoopData::getEventLoop() const
	{
		return mEventLoop;
	}

	/////////////
    EventLoopThreadPool::EventLoopThreadPool()
        : mRunIOLoop(false)
        , mNext(0)
    {

    }

    EventLoopThreadPool::~EventLoopThreadPool()
    {
        stop();
    }

    void EventLoopThreadPool::setThreadNum(uint16_t threadnum)
    {
        if (mRunIOLoop)
            return;

        mIOLoopDatas.resize(threadnum);
    }

    const std::shared_ptr<EventLoop>& EventLoopThreadPool::getNextLoop()
    {
		assert(!mIOLoopDatas.empty());
        
        auto ioLoopData = mIOLoopDatas[mNext];
        ++mNext;
        if (static_cast<size_t>(mNext) >= mIOLoopDatas.size())
        {
            mNext = 0;
        }
            
        return ioLoopData->getEventLoop();
    }

    void EventLoopThreadPool::start()
    {
        if (mRunIOLoop)
            return;
        
		mRunIOLoop = true;
        for (auto& v : mIOLoopDatas)
        {
            auto eventLoop = std::make_shared<EventLoop>();
            v = IOLoopData::Create(eventLoop, std::make_shared<std::thread>([this,
                eventLoop]()
            {
                while (mRunIOLoop)
                {
                    //auto timeout = std::chrono::milliseconds(sDefaultLoopTimeOutMS);
                    eventLoop->loop(LOOPTIMEOUTMS);

                }
            }));
        }
    }

    void EventLoopThreadPool::stop()
    {
        mRunIOLoop = false;
        for (const auto& v : mIOLoopDatas)
        {
            v->wakeupLoop();
            v->joinableThread();
        }

        mIOLoopDatas.clear();
    }
}