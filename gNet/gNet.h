#ifndef __GNET_H__
#define __GNET_H__

#include <stdint.h>
#include <memory>
#include <string>

namespace gNet {
    enum DataModel
    {
        PullData = 0,        // call function update()
        PushData,            // 
    };

    enum NetEvent
    {
        NET_Connected = 0x100,
        NET_PushData,
        NET_Disconnected,
    };

	enum NetStatus
	{
		Net_OK = 0,
	};

    class NetPackParse
    {
    public:
        typedef NetPackParse* PTR;
    public:
        virtual uint16_t parseData(const char *buf, uint32_t inLen,uint32_t *outLen) = 0;
    };

    class NetConnection
    {
    public:
        typedef             std::shared_ptr<NetConnection> PTR;
        virtual             ~NetConnection(void) = default;
        virtual uint32_t     getSessionID() = 0;
        virtual std::string  getIP() = 0;
		virtual void         sendData(const char *buf, uint32_t inLen) = 0;
		virtual void         shutDown() = 0;
    };

    typedef void(*NetCallback)(const NetConnection::PTR conn, uint32_t netEvent, const char* pBuf, uint32_t unLen, void* pUserData);
    
    class TcpServer;
    class NetTcpServer
    {
    public:
		NetTcpServer(uint32_t maxClient = 10000,
			DataModel model = DataModel::PushData,
			uint32_t maxPackSize = 1024 * 0x400,
			uint32_t maxSendSize = 32 * 0x400,
			uint32_t maxRecvSize = 16 * 0x400);

        virtual ~NetTcpServer(void) = default;
        void setPackarse(NetPackParse::PTR parse);
		void setCallback(NetCallback callBack, void * pUserData = nullptr);
        bool startService(uint16_t port, uint16_t threadnum, bool isIpv6 = false);
        void update();
    private:
		std::unique_ptr<TcpServer> mImpl;
    };
	
	
	class TcpClient;
	class NetTcpClient
	{
	public:
		NetTcpClient(DataModel model = DataModel::PushData,
			uint32_t maxSendSize = 32 * 0x400,
			uint32_t maxRecvSize = 16 * 0x400);

		virtual ~NetTcpClient(void) = default;
		void setPackarse(NetPackParse::PTR parse);
		void setCallback(NetCallback callBack, void * pUserData = nullptr);
		uint32_t connectServer(std::string ip, uint16_t port, bool isIpv6 = false);
		void update();
	private:
		std::unique_ptr<TcpClient> mImpl;
	};
}

#endif