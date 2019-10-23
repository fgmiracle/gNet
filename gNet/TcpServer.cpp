#include "TcpServer.h"

namespace gNet {

//#define DEFBUFLEN sizeof(int16_t)

	TcpServer::TcpServer(uint32_t maxClient,
		DataModel model,
		uint32_t maxPackSize,
		uint32_t maxSendSize,
		uint32_t maxRecvSize)
            : bRunning(false)
            , mModel(model)
            , mNetPackParse(nullptr)
			, maxPackSize(maxPackSize)
			/*, mSessionID(0)*/
    {
        UNUSE(maxClient);
        UNUSE(maxSendSize);
        UNUSE(maxRecvSize);
#ifdef _WIN32
        WORD version = MAKEWORD(2, 2);
        WSADATA d;
        if (WSAStartup(version, &d) != 0)
        {
            assert(0);
        }
#endif 
		//netCallBack = std::move(callBack);//std::bind(callBack, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, pUserData);

    }

    TcpServer::~TcpServer()
    {
#ifdef _WIN32
        WSACleanup();
#endif
        stopServer();
    }

	void TcpServer::setCallback(NetCallback &&callBack)
	{
		netCallBack = std::move(callBack);
	}

	void TcpServer::setPackarse(NetPackParse::PTR parse)
	{
		mNetPackParse = parse;
	}

    void TcpServer::stopServer()
    {
        bRunning = false;
        mMainLoop.wakeup();
        if (mListenThread->joinable())
        {
            mListenThread->join();
        }

        handlerThreads->stop();
    }

    bool TcpServer::startServer(uint16_t port, uint16_t threadnum, bool isIpv6 )
    {   
        bRunning = true;
        //mListener = std::make_unique<Listener>(&mMainLoop);
        mListener.reset(new Listener(&mMainLoop));
        if (!mListener->initListener(port, std::bind(&TcpServer::addNewConnection,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),isIpv6))
        {
            std::cout << "initListener error " << std::endl;
            return false;
        }

		handlerThreads = std::make_shared<EventLoopThreadPool>();
        handlerThreads->setThreadNum(threadnum);
        handlerThreads->start();
        
		mListenThread = std::make_shared<std::thread>([this]() {
			while (bRunning)
			{
				//auto timeout = std::chrono::milliseconds(sDefaultLoopTimeOutMS);
				mMainLoop.loop(LOOPTIMEOUTMS);
			}
		});
        
        return true;
    }

    void TcpServer::addNewConnection(gNetfd sock, uint32_t cliPort, std::string &cliIp)
    {
        char buf[64];
        snprintf(buf, sizeof buf, "%s-%d", cliIp.c_str(), cliPort);
        std::string name = buf;

        const auto& ioLoop = handlerThreads->getNextLoop();
        auto conn = std::make_shared<SocketChannel>(ioLoop.get(), sock, name, cliIp, cliPort, GETSESSIONID());
        
        conn->setDataCallback([this](LinearBuffer* buf, Channel::ChannelPtr conn) {
			if (buf->readableBytes() > maxPackSize)
			{
				std::cout << conn->getName() << " recv buff is full. will close it" << std::endl << std::flush;
				conn->shutDown();
				return;
			}
            if (mNetPackParse != nullptr)
            {
                uint32_t bufLen = 0;
                uint16_t offset = mNetPackParse->parseData(buf->beginRead(), buf->readableBytes(),&bufLen);
                if (bufLen + offset >= buf->readableBytes())
                {
                    buf->hasRead(offset);

                    if (mModel == PushData)
                    {
                        netCallBack(conn, NET_PushData, buf->beginRead(), bufLen);
                    }
                    else
                    {
                        PullInfo updateMsg;
                        updateMsg.conn = conn;
                        updateMsg.model = NET_PushData;
                        updateMsg.msg = std::move(std::string(buf->beginRead(),bufLen));
                        while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
                        }
                    }
                    
                    buf->hasRead(bufLen);
                }
            }
            else if (buf->readableBytes() >= DEFBUFLEN)
            {
                uint16_t bufLen = buf->readUint16();
                if ( buf->readableBytes() + DEFBUFLEN >= bufLen )
                {
                    buf->hasRead(DEFBUFLEN);
                    if (mModel == PushData)
                    {
                        netCallBack(conn, NET_PushData, buf->beginRead(), bufLen);
                    }
                    else
                    {
                        PullInfo updateMsg;
                        updateMsg.conn = conn;
                        updateMsg.model = NET_PushData;
                        updateMsg.msg = std::move(std::string(buf->beginRead(),bufLen));
                        while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
                        }
                    }

                    buf->hasRead(bufLen);
                }
            }
        });

        conn->setDisCallback([this](Channel::ChannelPtr conn){
            if (mModel == PushData)
            {
                netCallBack(conn, NET_Disconnected, nullptr, 0);
            }
            else
            {
                PullInfo updateMsg;
                updateMsg.conn = conn;
                updateMsg.model = NET_Disconnected;
                while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
                }
            }

			this->removeConnection(conn);
        });

        conn->setConCallback([this](Channel::ChannelPtr conn){
            if (mModel == PushData)
            {
                netCallBack(conn, NET_Connected, nullptr, 0);
            }
            else
            {
                PullInfo updateMsg;
                updateMsg.conn = conn;
                updateMsg.model = NET_Connected;
                while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
                }
            }
        });

		conn->setNetPack(maxPackSize);
        ioLoop->pushAsyncProc(std::bind(&Channel::connectEstablished, conn));

		// must in same thread(mMainLoop)
        connectionMap[name] = conn;
    }

    void TcpServer::removeConnection(Channel::ChannelPtr conn)
    {
		// maybe not in same thread(mMainLoop)
        mMainLoop.pushAsyncProc(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }

    void TcpServer::removeConnectionInLoop(Channel::ChannelPtr conn)
    {
		//std::cout << "NET_Disconnected --> " << conn->getName() << std::endl;
        size_t n = connectionMap.erase(conn->getName());
        (void)n;
        assert(n == 1);
        EventLoop* ioLoop = conn->getLoop();
        ioLoop->pushAsyncProc(std::bind(&Channel::connectDestroyed, conn));
    }

    void TcpServer::update()
    {
        PullInfo updateMsg;
        while (mUpdateMsg.try_dequeue(updateMsg)) {
            netCallBack(updateMsg.conn, updateMsg.model, updateMsg.msg.c_str(),static_cast<uint32_t>(updateMsg.msg.length()));
        }
    }
}