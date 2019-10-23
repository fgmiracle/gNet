#include "gNet/gNet.h"
#ifdef _WIN32
	#include <Windows.h>
#endif
#include <iostream>
#include <list>
#include <chrono>
#include <thread>
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

std::string sendBuf = "qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	qwwertyuuyoipasdfghjklmnbvczx123435465678679890ouik[dsfcxdasdscxcxxcvcbfgbcvwewertwrytuthnvbjuio9poioptyh4e52rw342c r4tv4tevd	\
	wertyui	\
";

uint64_t sendLen = 0;
uint64_t recvLen = 0;
uint64_t tatol = 0;

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
			std::cout << "NET_Connected --> " << conn->getIP() << std::endl;
			conn->sendData(sendBuf.c_str(), sendBuf.length());
			sendLen += sendBuf.length();
			//conn->shutDown();
		}
		else if (netEvent == NET_Disconnected) {
			std::cout << "NET_Disconnected --> " << conn->getIP() << std::endl;
		}
		else if (netEvent == NET_PushData) {
			conn->sendData(pBuf, unLen);
			recvLen += unLen;
			sendLen += unLen;
			tatol += unLen;
// 			if (tatol > 32*1024)
// 			{
// 				tatol = 0;
// 				std::cout << "NET_PushData --> " << conn->getIP() << " tatol sendLen:" << sendLen << ",tatol recvLen:" << recvLen << std::endl;
// 			}
			//std::cout << "NET_PushData --> " << conn->getIP() << " data:" << std::string(pBuf, unLen) << std::endl;
			//std::cout << "NET_PushData --> " << conn->getIP() << " tatol sendLen:" << pThis->sendLen << ",tatol recvLen:" <<pThis->recvLen << std::endl;
			//conn->shutDown();
		}
		else
		{
			std::cout << " unknown " << conn->getIP() << std::endl;
		}
	}

	void begin()
	{
		mClient = new gNet::NetTcpClient(DataModel::PushData);
		mClient->setCallback(callBack, this);
		mClient->setPackarse(this);
	}

	void connectS(std::string ip, uint16_t prot)
	{
		mClient->connectServer(ip, prot);
	}

	void update()
	{
		mClient->update();
	}

	gNet::NetTcpClient *mClient;
};

std::list<testNet *> testList;
int main(int argc, char* argv[])
{
	printf("sendBuf ->len:%d", sendBuf.length());
	for (int i = 0; i < 50; i++)
	{
		testNet *tnet = new testNet();
		tnet->begin();
		tnet->connectS("192.168.100.232", 8088);
	}
	
	auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		auto tt2 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		if (tt2 - tt >= 100)
		{
			std::cout<< " tatol sendLen:" << sendLen << ",tatol recvLen:" << recvLen << std::endl;
			break;
		}
	}

	system("pause");
	return 0;
}