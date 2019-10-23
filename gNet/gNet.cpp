#include "gNet.h"
#include "TcpServer.h"
#include "TcpClient.h"

namespace gNet {

	NetTcpServer::NetTcpServer(uint32_t maxClient,
		DataModel model ,
		uint32_t maxPackSize,
		uint32_t maxSendSize,
		uint32_t maxRecvSize)
	{
		if (nullptr == mImpl)
		{
			//mImpl = std::make_unique<TcpServer>(maxClient, model, maxPackSize, maxSendSize, maxRecvSize);
			//std::unique_ptr<TcpServer> tmp(new TcpServer(maxClient, model, maxPackSize, maxSendSize, maxRecvSize));
			std::cout << "start server! " << std::endl;
			mImpl.reset(new TcpServer(maxClient, model, maxPackSize, maxSendSize, maxRecvSize));
		}
	}

	void NetTcpServer::setCallback(NetCallback callBack, void * pUserData)
	{
		mImpl->setCallback(std::bind(callBack, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, pUserData));
	}

    bool NetTcpServer::startService(uint16_t port, uint16_t threadnum, bool isIpv6)
    {
		if (mImpl)
		{
			return mImpl->startServer(port, threadnum, isIpv6);
		}

		return false;
    }

    void NetTcpServer::setPackarse(NetPackParse::PTR parse)
    {
        mImpl->setPackarse(parse);
    }

    void NetTcpServer::update()
    {
		mImpl->update();
    }
    
	//---------------------------------------------------------
	//--------------------------------------------------------
	
	NetTcpClient::NetTcpClient(DataModel model,
		uint32_t maxSendSize,
		uint32_t maxRecvSize)
		: mImpl(nullptr)
	{
		if (nullptr == mImpl)
		{
			//mImpl = std::make_unique<TcpClient>(model, maxSendSize, maxRecvSize);
			mImpl.reset(new TcpClient(model, maxSendSize, maxRecvSize));
		}
	}

	void NetTcpClient::setPackarse(NetPackParse::PTR parse)
	{
		mImpl->setPackarse(parse);
	}

	void NetTcpClient::setCallback(NetCallback callBack, void * pUserData)
	{
		mImpl->setCallback(std::bind(callBack, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, pUserData));
	}

	uint32_t NetTcpClient::connectServer(std::string ip, uint16_t port, bool isIpv6)
	{
		return mImpl->connectServer(ip, port, isIpv6);
	}

	void NetTcpClient::update()
	{

	}
	
}