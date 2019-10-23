#include "Connector.h"
#include "SocketChannel.h"
#include "Iocp.h"
#include "..\epoll\Connector.h"

namespace gNet {

#ifndef _NTDEF_
	typedef LONG NTSTATUS;
	typedef NTSTATUS *PNTSTATUS;
#endif

#ifndef NT_SUCCESS
# define NT_SUCCESS(status) (((NTSTATUS) (status)) >= 0)
#endif

	Connector::Connector(EventLoop *loop, ConnectCallBack &&cb)
		: fCallBack(std::move(cb))
		, pLoop (loop)
		, pfnConnectEx(nullptr)
	{
		mConnectList.clear();
	}

	uint32_t Connector::connectRemote(EventLoop *loop, std::string ip, uint16_t port, bool isIpv6)
	{
		//gNetfd cliSocket = Socket::createSocket(isIpv6);
		gNetfd cliSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (pfnConnectEx == nullptr)
		{
			DWORD dwBytes = 0;
			GUID GuidConnectEx = WSAID_CONNECTEX;
			if (SOCKET_ERROR == WSAIoctl(
				cliSocket,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx,
				sizeof(GuidConnectEx),
				&pfnConnectEx,
				sizeof(pfnConnectEx),
				&dwBytes,
				NULL,
				NULL))
			{
				std::cout << "WSAIoctl get ConnectEx Pointer failed ‚error: " << Socket::socketErr() << std::endl;
				Socket::closeSocket(cliSocket);
				return 0;
			}
		}

		if (pfnConnectEx == nullptr)
		{
			std::cout << "pfnConnectEx is null" << std::endl << std::flush;
			return 0;
		}

		sockaddr_storage addRemote = { 0 };
		DWORD bytes;
		
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

		std::shared_ptr<OverLappedConnect> overlapped = std::make_shared<OverLappedConnect>();

		ConectStruct connInfo;
		connInfo.conn = conn;
		connInfo.mOverLappeConn = overlapped;

		//pLoop->pushAsyncProc(std::bind(&Connector::addConn, this, connInfo));
		addConn(connInfo);

		if (!pfnConnectEx(cliSocket, (const struct sockaddr*)&addrRemote, sizeof(addrRemote), nullptr, 0, &bytes, &(overlapped->mOverLapped)))
		{
			if (Socket::socketErr() != ERROR_IO_PENDING)
			{
				std::cout << "post Connect fail.error: " << Socket::socketErr() << std::endl;
				pLoop->pushAsyncProc(std::bind(&Connector::removeConn, this, conn));
				return 0;
			}
		}
		
		return session;
	}

	void Connector::addConn(ConectStruct &connInfo)
	{
		auto conn = connInfo.conn;
		SpinLock lock(m_Lock);
		mConnectList[conn->getFd()] = std::move(connInfo);
		
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
			auto connectOverlappd = mConnectList[conn->getFd()].mOverLappeConn;
			removeConn(conn);
		
			succ = NT_SUCCESS((NTSTATUS)(connectOverlappd->mOverLapped.Internal));
			if (succ && setsockopt(conn->getFd(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) != 0)
			{
				std::cout << "setsockopt(SO_UPDATE_CONNECT_CONTEXT) fail.error: " << Socket::socketErr() << std::endl << std::flush;
				succ = false;
			}
		}

		if (fCallBack)
		{
			fCallBack(conn, succ);
		}
	}
}