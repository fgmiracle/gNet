#include "TcpClient.h"
namespace gNet {

	TcpClient::TcpClient(DataModel model, uint32_t maxSendSize, uint32_t maxRecvSize)
		: bRunning (false)
		, netCallBack(nullptr)
		, mModel (model)
		, maxPackSize(64*1024)
		, mConnector(nullptr)
	{
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
		handlerThreads = std::make_shared<EventLoopThreadPool>();
		handlerThreads->setThreadNum(1);
		handlerThreads->start();

		const auto& ioLoop = handlerThreads->getNextLoop();
		mConnector = std::make_shared<Connector>(ioLoop.get(),std::bind(&TcpClient::addNewConnection,this, std::placeholders::_1, std::placeholders::_2));

	}

	TcpClient::~TcpClient()
	{
#ifdef _WIN32
		WSACleanup();
#endif
		clearClients();
	}

	void TcpClient::setCallback(NetCallback &&callBack)
	{
		netCallBack = std::move(callBack);
	}

	void TcpClient::setPackarse(NetPackParse::PTR parse)
	{
		mNetPackParse = parse;
	}

	uint32_t TcpClient::connectServer(std::string ip, uint16_t port, bool isIpv6)
	{
		const auto& ioLoop = handlerThreads->getNextLoop();

		return mConnector->connectRemote(ioLoop.get(), ip, port, isIpv6);
	}

	void TcpClient::update()
	{

	}

	void TcpClient::clearClients()
	{
		handlerThreads->stop();
		connectionMap.clear();
	}

	void TcpClient::addNewConnection(Channel::ChannelPtr conn, bool succ)
	{
		if (!succ)
		{
			netCallBack(conn, NET_Disconnected, nullptr, 0);
			return;
		}

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
				uint16_t offset = mNetPackParse->parseData(buf->beginRead(), buf->readableBytes(), &bufLen);
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
						updateMsg.msg = std::move(std::string(buf->beginRead(), bufLen));
						while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
						}
					}

					buf->hasRead(bufLen);
				}
			}
			else if (buf->readableBytes() >= DEFBUFLEN)
			{
				uint16_t bufLen = buf->readUint16();
				if (buf->readableBytes() + DEFBUFLEN >= bufLen)
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
						updateMsg.msg = std::move(std::string(buf->beginRead(), bufLen));
						while (!mUpdateMsg.enqueue(std::move(updateMsg))) {
						}
					}

					buf->hasRead(bufLen);
				}
			}
		});

		conn->setDisCallback([this](Channel::ChannelPtr conn) {
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

		conn->setConCallback([this](Channel::ChannelPtr conn) {
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
		conn->getLoop()->pushAsyncProc(std::bind(&Channel::connectEstablished, conn));

		connectionMap[conn->getName()] = conn;

	}

	void TcpClient::removeConnection(Channel::ChannelPtr conn)
	{
		SpinLock lock(m_Lock);
		removeConnectionInLoop(conn);
		//mMainLoop.pushAsyncProc(std::bind(&TcpClient::removeConnectionInLoop, this, conn));
	}

	void TcpClient::removeConnectionInLoop(Channel::ChannelPtr conn)
	{
		std::cout << "NET_Disconnected --> " << conn->getName() << std::endl;
		size_t n = connectionMap.erase(conn->getName());
		(void)n;
		assert(n == 1);
		EventLoop* ioLoop = conn->getLoop();
		ioLoop->pushAsyncProc(std::bind(&Channel::connectDestroyed, conn));
	}
}