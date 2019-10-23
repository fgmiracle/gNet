#include "gNet/gNet.h"
#ifdef _WIN32
	#include <Windows.h>
#endif
#include <list>
#include <chrono>
#include <thread>
#include <iostream>
using namespace gNet;

class testPackParse : public NetPackParse
{
public:
	uint16_t parseData(const char *buf, uint32_t inLen, uint32_t *outLen)
	{
		*outLen = inLen;
		return 0;
	}
};

testPackParse testUnpack;

class testNet : public NetPackParse
{
public:
	uint16_t parseData(const char *buf, uint32_t inLen, uint32_t *outLen)
	{
		*outLen = inLen;
		return 0;
	}

	static void callBack(const NetConnection::PTR conn, uint32_t netEvent, const char* pBuf, uint32_t unLen, void* pUserData)
	{
		testNet * pThis = (testNet*)pUserData;
		if (netEvent == NET_Connected) {
			std::cout << "NET_Connected --> " << conn->getIP() << " ,Session = " << conn->getSessionID()<< std::endl;
			//conn->shutDown();
		}
		else if (netEvent == NET_Disconnected) {
			std::cout << "NET_Disconnected --> " << conn->getIP() << std::endl;
		}
		else if (netEvent == NET_PushData) {
			//std::cout << "NET_PushData --> " << conn->getIP() << " data" << std::string(pBuf, unLen) << std::endl;
			conn->sendData(pBuf, unLen);
			//conn->shutDown();
		}
		else
		{
			std::cout << " unknown " << conn->getIP() << std::endl;
		}
	}

	void begin()
	{
		mServer = new gNet::NetTcpServer(10000);
		mServer->setCallback(callBack, this);
		mServer->setPackarse(this);
		mServer->startService(8088, 1);
	}

	void update()
	{
		//mServer->update();
	}

	gNet::NetTcpServer *mServer;
};

int main(int argc, char* argv[])
{
	testNet tnet;
	tnet.begin();
	while (true)
	{
		tnet.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return 0;
}