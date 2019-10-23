#ifndef __GNET_CONNECTOR_H__
#define __GNET_CONNECTOR_H__

#include "gNet/SessionID.h"
#include "SocketChannel.h"
#include "gNet/utils/ThreadLock.hpp"

namespace gNet {

	class EventLoop;
	class SocketChannel;
	class Connector
	{
		struct ConectStruct
		{
			std::shared_ptr<SocketChannel>		conn;
			std::shared_ptr<OverLappedConnect> mOverLappeConn;
		};

	public:
		using ConnectCallBack = std::function<void(Channel::ChannelPtr,bool)>;
		typedef std::shared_ptr<Connector> ConnectorPtr;

	public:
		Connector(EventLoop *loop, ConnectCallBack &&cb);
		uint32_t		connectRemote(EventLoop *loop, std::string ip, uint16_t port, bool isIpv6 = false);

	private:
		void			addConn(ConectStruct &connInfo);
		void			removeConn(Channel::ChannelPtr conn);
		void			handleConnect(Channel::ChannelPtr conn);
		void			handleConnectInLoop(Channel::ChannelPtr conn);
	private:
		LPFN_CONNECTEX										pfnConnectEx;
		ConnectCallBack										fCallBack;
		EventLoop											*pLoop;
		std::map<gNetfd, ConectStruct>						mConnectList;
		std::atomic_flag									m_Lock = ATOMIC_FLAG_INIT;
	};
}

#endif
