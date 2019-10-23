#include "Listener.h"
#include "Epoll.h"

namespace gNet {

    class AcceptChannel final : public Channel, public NonCopyable
    {
    public:
        using ACCEPT_CALLBACK = std::function<void()>;
		AcceptChannel(EventLoop *pLoop, gNetfd sockfd ,ACCEPT_CALLBACK && cb)
			:Channel(pLoop, sockfd)
		{ mCallback = std::move(cb); }

		void    sendData(const std::string &msg) override {UNUSE(msg);}
		void    connectDestroyed() override {}
		void    connectEstablished() override {}
		void    sendData(const char *buf, uint32_t inLen) override {UNUSE(buf);UNUSE(inLen);};
		void    shutDown() override {}
        void    enableReading() { mEvents |= mReadEvent; pLoop->updateChannel(this); }
    private:
		void    canSend() override {}
		void    canRecv() override {
			if (mCallback)
				mCallback();
		}
		void    doConnect() override {}
        void    doClose(bool bCheck ) override { UNUSE(bCheck);}

		void    doAccept() override
        {
			
        }

    private:
        ACCEPT_CALLBACK     mCallback;
	};

    // listener impl

    Listener::Listener(EventLoop *loop)
        : pLoop(loop)
        , mIsIPv6(false)
    {
    }

    Listener::~Listener()
	{
    }

    bool Listener::initListener(uint16_t port, ACCEPT_CALLBACK callback, bool isIpv6)
    {

        if(mChannel.get() != nullptr)
            return false;
        
        assert(pLoop != nullptr);

        gNetfd mListenfd = Socket::createSocket(isIpv6);

        Socket::setReuseAddr(mListenfd);
        Socket::setReusePort(mListenfd);

        if (!Socket::bind(mListenfd, port, nullptr, isIpv6))
            return false;

        if (!Socket::listen(mListenfd))
            return false;

   

        mAcceptCallback = callback;
        
 		//mChannel = std::make_unique<AcceptChannel>(pLoop, mListenfd, std::bind(&Listener::handleAccept, this));
        mChannel.reset(new AcceptChannel(pLoop, mListenfd, std::bind(&Listener::handleAccept, this)));
        pLoop->attach(mChannel.get());
		//bool b = pLoop->attach(mChannel.get());
        mChannel->enableReading();
        mIsIPv6 = isIpv6;

        return true;
    }

	void Listener::handleAccept()
	{
        //std::cout << "---> handleAccept --- >" << Socket::socketErr() << std::endl;
		Socket::sockaddr_all sa;
		socklen_t len = sizeof(sa);
		gNetfd clientFD = Socket::acceptEx(mChannel->getFd(), &sa.s, &len);
		if (clientFD == SOCKET_INVALID)
		{
			std::cout << "accept error:" << Socket::socketErr() << std::endl;
			return ;
		}

		std::string sClientip;
		uint16_t    uClientPort = 0;
		Socket::getAddr(&sa, sClientip, uClientPort);

		mAcceptCallback(clientFD, uClientPort, sClientip);
	}
}