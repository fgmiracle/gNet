#ifndef __GNET_TCPCLIENT_H__
#define __GNET_TCPCLIENT_H__

#include "gNet.h"
#include "EventLoopThreadPool.h"

#ifdef _WIN32
#include "iocp/Iocp.h"
#include "iocp/SocketChannel.h"
#include "iocp/Connector.h"
#else
#include "epoll/Epoll.h"
#include "epoll/Connector.h"
#include "epoll/SocketChannel.h"
#endif

namespace gNet {

	class TcpClient
	{
	public:
		using NetCallback = std::function<void(const NetConnection::PTR, uint32_t, const char*, uint32_t)>;
		typedef struct PullInfoSTRUCT
		{
			std::string msg;
			NetEvent model;
			NetConnection::PTR conn;
		} PullInfo;

	public:
		TcpClient(DataModel model,
			uint32_t maxSendSize,
			uint32_t maxRecvSize);
		~TcpClient();

		void		setCallback(NetCallback &&callBack);
		void		setPackarse(NetPackParse::PTR parse);
		uint32_t    connectServer(std::string ip, uint16_t port, bool isIpv6);
		void		update();

	private:
		void		clearClients();
		void		addNewConnection(Channel::ChannelPtr conn,bool succ);
		void		removeConnection(Channel::ChannelPtr conn);
		void		removeConnectionInLoop(Channel::ChannelPtr conn);

	private:
		bool                                    bRunning;
		NetCallback                             netCallBack;
		DataModel                               mModel;
		NetPackParse::PTR                       mNetPackParse;
		std::shared_ptr<EventLoopThreadPool>    handlerThreads;
		moodycamel::ConcurrentQueue<PullInfo>   mUpdateMsg;
		std::map<std::string, Channel::ChannelPtr> connectionMap;
		uint32_t								maxPackSize;
		std::atomic_flag						m_Lock = ATOMIC_FLAG_INIT;
		Connector::ConnectorPtr					mConnector;
	};
}

#endif
