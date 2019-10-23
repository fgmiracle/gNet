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
			bool								succ;
			std::shared_ptr<SocketChannel>		conn;
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
		void			handleConnect(Channel::ChannelPtr conn );
		void			handleConnectInLoop(Channel::ChannelPtr conn);
		//
		void			handleConneting(std::shared_ptr<SocketChannel> conn);
		void			handleConneted(std::shared_ptr<SocketChannel> conn);

		void			connectingSocketWrite(Channel::ChannelPtr conn);

	private:
		ConnectCallBack										fCallBack;
		EventLoop											*pLoop;
		std::map<gNetfd, ConectStruct>						mConnectList;
		std::atomic_flag									m_Lock = ATOMIC_FLAG_INIT;
	};
}

#endif
