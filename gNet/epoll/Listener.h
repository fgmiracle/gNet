#ifndef __GNET_LISTENER_H__
#define __GNET_LISTENER_H__

#include "gNet/Channel.h"

namespace gNet {
    class EventLoop;
    class AcceptChannel;
    class Listener
    {
    public:
        using ACCEPT_CALLBACK =  std::function<void(gNetfd, uint32_t, std::string &)>;
    public:
        Listener(EventLoop *loop);
        ~Listener();
        bool    initListener(uint16_t port, ACCEPT_CALLBACK callback,bool isIpv6 = false);

    private:
        void handleAccept();

    private:
        ACCEPT_CALLBACK                                 mAcceptCallback;
        EventLoop                                       *pLoop;
		std::unique_ptr<AcceptChannel>					mChannel;
        bool											mIsIPv6;
    };
}

#endif