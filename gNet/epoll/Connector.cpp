#include "Connector.h"
#include "SocketChannel.h"
#include "Epoll.h"

namespace gNet {

	Connector::Connector(EventLoop *loop, ConnectCallBack &&cb)
		: fCallBack(std::move(cb))
		, pLoop (loop)
	{
		mConnectList.clear();
	}

	uint32_t Connector::connectRemote(EventLoop *loop, std::string ip, uint16_t port, bool isIpv6)
	{
		gNetfd cliSocket = Socket::createSocket(isIpv6);
		Socket::setNonBlock(cliSocket);

		sockaddr_storage addrRemote;
		if (!Socket::fillAddr(ip.c_str(), port, &addrRemote, isIpv6))
		{
			Socket::closeSocket(cliSocket);
			return 0;
		}

		if (!Socket::bindLocal(cliSocket, isIpv6))
		{
			Socket::closeSocket(cliSocket);
			return 0;
		}
		
		char buf[64];
		snprintf(buf, sizeof buf, "%s-%d", ip.c_str(), port);
		std::string name = buf;
		uint32_t session = GETSESSIONID();
		auto conn = std::make_shared<SocketChannel>(loop, cliSocket, name, ip, port, session);

		conn->setConCallback([this](Channel::ChannelPtr conn) {
			this->handleConnect(conn);
		});


		ConectStruct connInfo;
		connInfo.conn = conn;
		connInfo.succ = false;
		addConn(connInfo);

		int32_t err = ::connect(cliSocket, (struct sockaddr*)&addrRemote, sizeof(addrRemote));
		int32_t currno = (err == 0) ? 0 : Socket::socketErr();
		switch (currno)
		{
			// 链接成功
		case 0:
			connInfo.succ = true;
			handleConneted(conn);
			break;
			// 链接进行中
		case EINPROGRESS:
			// 系统调用被中断？？
		case EINTR:
			connInfo.succ = true;
			handleConneting(conn);
			break;
		case EISCONN:
			std::cout << "connect error! socketfd have connect" << std::endl;
			break;
		case EAGAIN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		case ECONNREFUSED:
		case ENETUNREACH:
			std::cout << "connect error in Connector::startInLoop currno:" << currno << std::endl;
			handleConnect(conn);
			break;

		case EACCES:
		case EPERM:
		case EAFNOSUPPORT:
		case EALREADY:
		case EBADF:
		case EFAULT:
		case ENOTSOCK:
			std::cout << "connect error in Connector::startInLoop currno:" << currno << std::endl;
			handleConnect(conn);
			break;

		default:
			std::cout << "Unexpected error in Connector::startInLoop " << currno <<std::endl;
			handleConnect(conn);
			break;
		}

		return session;
	}

	void Connector::addConn(ConectStruct &connInfo)
	{
		auto conn = connInfo.conn;
		SpinLock lock(m_Lock);
		mConnectList[conn->getFd()] = std::move(connInfo);
	}

	void Connector::handleConneting(std::shared_ptr<SocketChannel> conn)
	{
		pLoop->pushAsyncProc([this, conn]() {
			conn->setStatus(Channel::kConnecting);
			conn->setWriteCallBack(std::bind(&Connector::connectingSocketWrite, this,std::placeholders::_1));
			conn->enableWriting();
		});
	}

	void Connector::handleConneted(std::shared_ptr<SocketChannel> conn)
	{
		handleConnect(conn);
	}

	void Connector::connectingSocketWrite(Channel::ChannelPtr conn)
	{
		pLoop->pushAsyncProc(std::bind(&Connector::handleConnectInLoop, this, conn));
	}

	void Connector::removeConn(Channel::ChannelPtr conn)
	{
		size_t n = mConnectList.erase(conn->getFd());
		(void)n;
		assert(n == 1);
	}

	void Connector::handleConnect(Channel::ChannelPtr conn)
	{
		pLoop->pushAsyncProc(std::bind(&Connector::handleConnectInLoop, this, conn));
	}

	void Connector::handleConnectInLoop(Channel::ChannelPtr conn)
	{		
		bool succ = false;
		{
			SpinLock lock(m_Lock);
			assert(mConnectList.find(conn->getFd()) != mConnectList.end());
			succ = mConnectList[conn->getFd()].succ;
			removeConn(conn);
		}
		if (fCallBack)
		{
			fCallBack(conn, succ);
		}
	}
}