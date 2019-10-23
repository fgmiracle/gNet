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
        bool postAccept(OverLapped *overLapped);
        void handleAccept(OverLapped *overLapped);
    private:
        ACCEPT_CALLBACK                                 mAcceptCallback;
        EventLoop                                       *pLoop;
        std::vector<std::shared_ptr<OverLappedAccept>>  mAcceptList;
		std::unique_ptr<AcceptChannel>					mChannel;
        
        //////////////
        LPFN_ACCEPTEX                   m_pfnAcceptEx;
        LPFN_GETACCEPTEXSOCKADDRS       m_pfnGetAcceptExSockAddrs;
        bool                            mIsIPv6;
    };
}

#endif