#ifndef __GNET_EVNETLOOPTHREADPOOL_H__
#define __GNET_EVNETLOOPTHREADPOOL_H__

#include <gNet/utils/common.h>

namespace gNet {
	class EventLoop;
    class IOLoopData;
    class EventLoopThreadPool : NonCopyable
    {
    public:
        EventLoopThreadPool();
        ~EventLoopThreadPool();
        void        setThreadNum(uint16_t threadnum = 1);
        const std::shared_ptr<EventLoop>& getNextLoop();
        
        void        start();
        void        stop();

    private:
        std::vector<std::shared_ptr<IOLoopData>>    mIOLoopDatas;
        bool                                        mRunIOLoop;
        uint16_t                                    mNext;
    };
}

#endif