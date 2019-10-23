#include "Listener.h"
#include "Iocp.h"

namespace gNet {

    class AcceptChannel final : public Channel, public NonCopyable
    {
    public:
        using ACCEPT_CALLBACK = std::function<void(OverLapped*)>;
		AcceptChannel(EventLoop *pLoop, gNetfd sockfd ,ACCEPT_CALLBACK && cb)
			:Channel(pLoop, sockfd)
		{ mCallback = std::move(cb); }

		void    sendData(const std::string &msg) override {}
		void    connectDestroyed() override {}
		void    connectEstablished() override {}
		void    sendData(const char *buf, uint32_t inLen) override {};
		void    shutDown() override {}
    private:
		void    canSend() override {}
		void    canRecv() override {}
		void    doConnect() override {}
        void    doClose(bool bCheck ) override {}

		void    doAccept(OverLapped *overLapped)
        {
            if (mCallback)
                mCallback(overLapped);
        }

    private:
        ACCEPT_CALLBACK     mCallback;
	};

    // listener impl

    Listener::Listener(EventLoop *loop)
        : pLoop(loop)
        , mIsIPv6(false)
    {
        mAcceptList.resize(5);
    }

    Listener::~Listener()
    {
        mAcceptList.clear();
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

        DWORD dwBytes = 0;
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

        if (SOCKET_ERROR == WSAIoctl(
            mListenfd,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &GuidAcceptEx,
            sizeof(GuidAcceptEx),
            &m_pfnAcceptEx,
            sizeof(m_pfnAcceptEx),
            &dwBytes,
            NULL,
            NULL))
        {
            std::cout << "WSAIoctl get AcceptEx Pointer failed ‚error: "<< Socket::socketErr() << std::endl;
            Socket::closeSocket(mListenfd);
            return false;
        }

        if (SOCKET_ERROR == WSAIoctl(
            mListenfd,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            &GuidGetAcceptExSockAddrs,
            sizeof(GuidGetAcceptExSockAddrs),
            &m_pfnGetAcceptExSockAddrs,
            sizeof(m_pfnGetAcceptExSockAddrs),
            &dwBytes,
            NULL,
            NULL))
        {
            std::cout << "WSAIoctl get GuidGetAcceptExSockAddrs Pointer failed ‚error: " << Socket::socketErr() << std::endl;
            Socket::closeSocket(mListenfd);
            return false;
        }

        mAcceptCallback = callback;
        
 		//mChannel = std::make_unique<AcceptChannel>(pLoop, mListenfd, std::bind(&Listener::handleAccept, this, std::placeholders::_1));
        mChannel.reset(new AcceptChannel(pLoop, mListenfd, std::bind(&Listener::handleAccept, this, std::placeholders::_1)));
        pLoop->attach(mChannel.get());
		//bool b = pLoop->attach(mChannel.get());

        mIsIPv6 = isIpv6;

        for (auto& v : mAcceptList)
        {
			//std::shared_ptr<char>(new char[200]);
			v = std::make_shared<OverLappedAccept>();//(std::bind(&Listener::handleAccept, this, std::placeholders::_1));
            postAccept(v.get());
        }

        return true;
    }

    //////////////////////////
    bool Listener::postAccept(OverLapped *overLapped)
    {
        //OverLappedAccept & accIO = channel->getOverLapped();
		overLapped->mSocket = Socket::createSocket(mIsIPv6);

        if (SOCKET_INVALID == overLapped->mSocket)
        {
            std::cout << "crete Accep  Socket failed . error: " << Socket::socketErr() << std::endl;
            return false;
        }

        if (mIsIPv6)
        {
            if (!m_pfnAcceptEx(mChannel->getFd(), overLapped->mSocket, (overLapped->mExtrabuf).get(), 0, sizeof(sockaddr_in6) + 16, sizeof(sockaddr_in6) + 16, nullptr, &(overLapped->mOverLapped)))
            {
                if (WSAGetLastError() != ERROR_IO_PENDING)
                {
                    std::cout << "post AcceptEx error.error: " << Socket::socketErr() << std::endl;
                    Socket::closeSocket(overLapped->mSocket);
                    return false;
                }
            }
        }
        else
        {
            if (!m_pfnAcceptEx(mChannel->getFd(), overLapped->mSocket, (overLapped->mExtrabuf).get(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, nullptr, &(overLapped->mOverLapped)))
            {
                if (WSAGetLastError() != ERROR_IO_PENDING)
                {
                    std::cout << "post AcceptEx error.error: " << Socket::socketErr() << std::endl;
                    Socket::closeSocket(overLapped->mSocket);
                    return false;
                }
            }
        }

        return true;
    }

    void Listener::handleAccept(OverLapped *overLapped)
    {
		//OverLappedAccept & accIO = channel->getOverLapped();
        int32_t     iLocalSockaddrLen = 0;
        int32_t     iRemoteSockaddrLen = 0;
        sockaddr*   pLocalSockAddr = NULL;
        sockaddr*   pRemoteSockAddr = NULL;
        std::string sClientip;
        uint16_t    uClientPort = 0;

        if (mIsIPv6)
        {
            m_pfnGetAcceptExSockAddrs((overLapped->mExtrabuf).get(), 0, sizeof(SOCKADDR_IN6) + 16, sizeof(SOCKADDR_IN6) + 16, (sockaddr **)&pLocalSockAddr, &iLocalSockaddrLen, (sockaddr **)&pRemoteSockAddr, &iRemoteSockaddrLen);
            char ip[32] = { 0 };
            inet_ntop(AF_INET6, &(((SOCKADDR_IN6*)pRemoteSockAddr)->sin6_addr), ip, sizeof(ip));
            uClientPort = ntohs(((sockaddr_in6*)pRemoteSockAddr)->sin6_port);
            sClientip = ip;
        }else
        {
            m_pfnGetAcceptExSockAddrs((overLapped->mExtrabuf).get(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (sockaddr **)&pLocalSockAddr, &iLocalSockaddrLen, (sockaddr **)&pRemoteSockAddr, &iRemoteSockaddrLen);
            sClientip = inet_ntoa(((sockaddr_in*)pRemoteSockAddr)->sin_addr);
            uClientPort = ntohs(((sockaddr_in*)pRemoteSockAddr)->sin_port);
        }

		gNetfd mListenfd = mChannel->getFd();
		setsockopt(overLapped->mSocket,
			SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			(char*)&mListenfd,
			sizeof(mListenfd));

        mAcceptCallback(overLapped->mSocket, uClientPort, sClientip);

        postAccept(overLapped);
    }
}