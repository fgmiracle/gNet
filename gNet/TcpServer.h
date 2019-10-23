#ifndef __GNET_TCPSERVER_H__
#define __GNET_TCPSERVER_H__

#include "gNet.h"
#include "EventLoopThreadPool.h"
#include "SessionID.h"

#ifdef _WIN32
#include "iocp/Iocp.h"
#include "iocp/Listener.h"
#include "iocp/SocketChannel.h"
#else
#include "epoll/Epoll.h"
#include "epoll/Listener.h"
#include "epoll/SocketChannel.h"
#endif

namespace gNet {

    class TcpServer
    {
    public:
        using NetCallback = std::function<void(const NetConnection::PTR, uint32_t, const char*, uint32_t )>;
		typedef struct PullInfoSTRUCT
		{
			std::string msg;
			NetEvent model;
			NetConnection::PTR conn;
		} PullInfo;

    public:
		TcpServer(uint32_t maxClient,
			DataModel model,
			uint32_t maxPackSize,
			uint32_t maxSendSize,
			uint32_t maxRecvSize);
        ~TcpServer();

		void    setCallback(NetCallback &&callBack);
        void    setPackarse(NetPackParse::PTR parse);
        bool    startServer(uint16_t port, uint16_t threadnum, bool isIpv6 );
        void    stopServer();
        void    update();
    private:
        void    addNewConnection(gNetfd sock, uint32_t cliPort, std::string &cliIp);
        void    removeConnection(Channel::ChannelPtr conn);
        void    removeConnectionInLoop(Channel::ChannelPtr conn);

// 		inline uint32_t getSessionID()
// 		{
// 			return ++mSessionID;
// 		}
    private:
        bool                                    bRunning;
        NetCallback                             netCallBack;
        DataModel                               mModel;
        NetPackParse::PTR                       mNetPackParse;
        std::unique_ptr<Listener>               mListener;
        std::shared_ptr<EventLoopThreadPool>    handlerThreads;
        EventLoop                               mMainLoop;
        std::shared_ptr<std::thread>            mListenThread;
        moodycamel::ConcurrentQueue<PullInfo>   mUpdateMsg;
        std::map<std::string, Channel::ChannelPtr> connectionMap;

		/////////////////////////////////////
		uint32_t								maxPackSize;
		//std::atomic<uint32_t>                   mSessionID;
    };
}

#endif