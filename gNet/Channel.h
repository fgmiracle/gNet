#ifndef __GNET_CHANNEL_H__
#define __GNET_CHANNEL_H__

#include "gNet/Socket.h"
#include "gNet.h"

namespace gNet {
    ////////////////////
#define DEFBUFLEN sizeof(int16_t)
    // win OVERLAPPED define
#ifdef _WIN32
    enum enOperationType
    {
        enOperationType_Accpet,
        enOperationType_Send,
        enOperationType_Recv,
        enOperationType_Connect,
		enOperationType_Wakeup,
    };

    class OverLapped
    {
	public:
		gNetfd					  mSocket;
		std::shared_ptr<char>	  mExtrabuf;
        OVERLAPPED                mOverLapped;
        WSABUF                    mWSABuffer[2];
	private:
        enOperationType			  mOperationType;
    public:
        OverLapped(enOperationType OperationType)
            :mOperationType(OperationType)
			, mSocket(SOCKET_INVALID)
        {
            memset(&mOverLapped, 0, sizeof(mOverLapped));
        }

		virtual ~OverLapped() = default ;

    public:
        enOperationType GetOperationType()
        {
            return mOperationType;
        }
    };

    class OverLappedSend : public OverLapped
    {

    public:
        OverLappedSend()
            : OverLapped(enOperationType_Send) {}
    };

    class OverLappedRecv : public OverLapped
    {
    public:
        OverLappedRecv()
            : OverLapped(enOperationType_Recv) {}
    };

    class OverLappedAccept : public OverLapped
    {
	public:
        OverLappedAccept()
            : OverLapped(enOperationType_Accpet) 
		{
			mExtrabuf = std::make_shared<char>(200);
		}
    };

    class OverLappedConnect : public OverLapped
    {
    public:
        OverLappedConnect()
            : OverLapped(enOperationType_Connect) 
		{
			
		}
    };
	class OverLappedWakeup : public OverLapped
	{
	public:
		OverLappedWakeup()
			: OverLapped(enOperationType_Wakeup)
		{

		}
	};
#endif

    class EventLoop;
    class LinearBuffer;
    class Channel: public NetConnection, public std::enable_shared_from_this<Channel>
    {
	public:
		enum Status {
			kDisconnected	= 0,
			kConnecting		= 1,
			kConnected		= 2,
			kDisconnecting	= 3,
			kClosed			= 4,
		};

    public:
		typedef std::shared_ptr<Channel> ChannelPtr;
		typedef std::function<void(LinearBuffer*, ChannelPtr)> dataCallback;
		typedef std::function<void(ChannelPtr)> disCallback;
		typedef std::function<void(ChannelPtr)> conCallback;
		typedef std::function<void(ChannelPtr)> writeCallback;

    public:
        Channel(EventLoop *pLoop = nullptr, 
			gNetfd sockfd = SOCKET_INVALID, 
			const std::string& nameArg = "", 
			const std::string& ip = "", 
			const uint32_t& port = 0,
			const uint32_t& session = 0)

            : pLoop(pLoop)
            , sockFD(sockfd)
			, mName(nameArg)
            , mPort(port)
			, mIP(ip)
			, mDataCallback(nullptr)
			, mDisCallback(nullptr)
			, mConCallback(nullptr)
            , mEvents(mNoneEvent)
            , mSessionID(session)
            , reqCount(0)
            , mStatus(kDisconnected)
            {}
        

        virtual void    sendData(const std::string &msg) = 0;
		EventLoop*		getLoop() { return pLoop; }
		std::string		getName() const { return mName; }
        gNetfd          getFd() const { return sockFD; };
        uint32_t        getSessionID() { return mSessionID;}
		std::string		getIP() { return mIP; };
		int32_t			getEvents() const { return mEvents; }
		void			setEvents(int32_t events) { mEvents = events; }

		virtual void	connectEstablished() = 0;
		virtual void	connectDestroyed() = 0;

		virtual void	setNetPack(uint32_t maxLen) { UNUSE(maxLen) ;}

		void			setDataCallback(dataCallback &&f) { mDataCallback = std::move(f); }
		void			setDisCallback(disCallback &&f) { mDisCallback = std::move(f); }
		void			setConCallback(conCallback &&f) { mConCallback = std::move(f); }

		void			setStatus(Status st) { mStatus = st; }
		Status			getStatus() { return mStatus; }
	protected:
#ifdef _WIN32
		virtual void    doAccept(OverLapped *overLapped) = 0;	
#else
		virtual void    doAccept() = 0;
#endif // _WIN32

		virtual void    doConnect() { if (mConCallback) mConCallback(shared_from_this()); }
		virtual void    doClose(bool bCheck = true) = 0;

        virtual void    canRecv() = 0;
        virtual void    canSend() = 0;
	protected:
        EventLoop               *pLoop;
        gNetfd                  sockFD;
		std::string				mName;
		uint32_t                mPort;
		std::string             mIP;

		dataCallback            mDataCallback;
		disCallback             mDisCallback;
		conCallback             mConCallback;
        
		int32_t                 mEvents;
        uint32_t                mSessionID;
        bool                    mClosing;
        std::atomic<uint32_t>   reqCount;
		std::atomic<Status>		mStatus;

        static const int16_t    mNoneEvent  = 0;
#ifdef _WIN32
        static const int16_t    mReadEvent  = POLLIN | POLLPRI;
        static const int16_t    mWriteEvent = POLLOUT;
#else
		static const int16_t    mReadEvent = EPOLLIN | EPOLLPRI;
		static const int16_t    mWriteEvent = EPOLLOUT;
#endif

	private:
        friend class EventLoop;
    };
}

#endif