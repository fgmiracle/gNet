#include "SocketChannel.h"
#include "Epoll.h"

namespace gNet {

	SocketChannel::SocketChannel(EventLoop *pLoop, gNetfd sockfd, const std::string& nameArg,
		const std::string& ip, const uint32_t& port,const uint32_t& session)
        :Channel(pLoop,sockfd, nameArg, ip, port, session)
    {
		if (!pLoop->attach(this))
		{
			std::cout << "attach loop fail! name ->" << nameArg << std::endl << std::flush;
		}

		//mSendBuf.initCapacity(32 * 1024);
    }

    SocketChannel::~SocketChannel()
    {
        if (sockFD != SOCKET_INVALID)
        {
            Socket::closeSocket(sockFD);
            sockFD = SOCKET_INVALID;
        }
    }

	void SocketChannel::sendData(const char *buf, uint32_t inLen)
	{
		if (mStatus != kConnected)
		{
			return;
		}

		sendData(std::string(buf, inLen));
	}

	void SocketChannel::shutDown()
	{
		pLoop->pushAsyncProc(std::bind(&SocketChannel::shutDownInLoop, this));
	}

    void SocketChannel::sendData(const std::string &msg)
    {
		if (mStatus != kConnected)
		{
			return;
		}

		if (pLoop->isInLoopThread())
		{
			sendInLoopStr(msg);
		}
		else
		{
			pLoop->pushAsyncProc(
				std::bind(&SocketChannel::sendInLoopStr, this, msg));
		}

    }

	void SocketChannel::shutDownInLoop()
	{
		if (mStatus != kConnected)
		{
			return;
		}

		std::cout << getName() << " shutDown " << std::endl << std::flush;
		int nResultCode = Socket::shutDown(sockFD);
		UNUSE(nResultCode);
		//doClose(false);
	}
	void SocketChannel::setNetPack(uint32_t maxLen)
	{
		UNUSE(maxLen);
		//mSendBuf.initCapacity(maxLen);
	}

	void SocketChannel::sendInLoopStr(const std::string &msg)
	{
		sendInLoopPtr(msg.c_str(), msg.length());
	}

	void SocketChannel::sendInLoopPtr(const void* data, size_t len)
	{
		int32_t nSend = 0;
		if (!isWriting() && mSendBuf.readableBytes() == 0)
		{
			nSend = ::write(sockFD, static_cast<const char*>(data), len);
			if (nSend <= 0 )
			{
				if (Socket::socketErr() != EWOULDBLOCK)
				{
					if (Socket::socketErr() == EPIPE || Socket::socketErr() == ECONNRESET)
					{
						std::cout << "send error = " << Socket::socketErr() << " ." << std::endl;
						doClose();
						return;
					}					
				}
			}
		}
		
		uint32_t remaining = len - nSend;
		if (remaining > 0)
		{
			mSendBuf.append(static_cast<const char*>(data) + nSend, remaining);
			// wait next send;
			if (!isWriting())
			{
				enableWriting();
			}
		}
	}

	void SocketChannel::connectEstablished()
	{
		mStatus = kConnected;
		
		Socket::setNoDelay(sockFD);
		Socket::setNonBlock(sockFD);
		Socket::setKeepAlive(sockFD);
		/*
		UCHAR sfcnm_flags = FILE_SKIP_SET_EVENT_ON_HANDLE | FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
		if (!SetFileCompletionNotificationModes((HANDLE)sockFD, sfcnm_flags))
		{
			std::cout << "SetFileCompletionNotificationModes --> error:"<< GetLastError() << std::endl << std::flush;
		}
		*/
		if (mConCallback)
		{
			mConCallback(shared_from_this());
		}

		enableReading();
	}

	void SocketChannel::connectDestroyed()
	{
	}

    void SocketChannel::doClose(bool bCheck)
    {
		UNUSE(bCheck);
		if (mStatus == kDisconnected || mStatus == kDisconnecting)
		{
			return;
		}

		mStatus = kDisconnecting;
		if (reqCount == 0)
		{
			pLoop->detach(this);
			mDisCallback(shared_from_this());
			mStatus = kDisconnected;
		}
		else
		{
			std::cout << getName() <<" will close->reqCpunt:" << reqCount << std::endl << std::flush;
		}
    }
	void SocketChannel::doSend()
    {
		  
    }

	void SocketChannel::doRecv()
    {
		
    }

	void SocketChannel::canRecv()
	{
		if (mStatus == kDisconnecting)
		{
			disableReading();
			if (mWriteCallBack)
			{
				mWriteCallBack(shared_from_this());
			}
			return;
		}
	
		char extrabuf[65536];
		struct iovec vec[2];
		const size_t writable = mRecvBuf.writableBytes();
		vec[0].iov_base = mRecvBuf.beginWrite();
		vec[0].iov_len = writable;
		vec[1].iov_base = extrabuf;
		vec[1].iov_len = sizeof(extrabuf);

		const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
		const int32_t nRecv = ::readv(sockFD, vec, iovcnt);

		if (nRecv <= 0)
		{
			std::cout << "readv fail.error:" << Socket::socketErr() << std::endl << std::flush;
			doClose();
			return;
		}
		else if (static_cast<size_t>(nRecv) <= writable) {
			mRecvBuf.hasWritten(nRecv);
		}
		else 
		{
			mRecvBuf.hasWritten(writable);
			mRecvBuf.append(extrabuf, nRecv - writable);
		}

		if (mDataCallback)
		{
			mDataCallback(&mRecvBuf, shared_from_this());
		}
	}

	void SocketChannel::canSend()
	{		

		if (isWriting())
		{
			size_t oldLen = mSendBuf.readableBytes();
			size_t nSend = ::write(sockFD, mSendBuf.beginRead(), oldLen);
			if (nSend <= 0)
			{
				if (Socket::socketErr() != EWOULDBLOCK)
				{
					if (Socket::socketErr() == EPIPE || Socket::socketErr() == ECONNRESET)
					{
						std::cout << "send error = " << Socket::socketErr() << " ." << std::endl;
						doClose();
						return;
					}
				}
			}
			else
			{
				mSendBuf.hasRead(nSend);
				if (mSendBuf.readableBytes() == 0)
				{
					disableWriting();
				}
			}
		}
		else
		{
			std::cout << "Connection session =  " << mSessionID << " is down, no more writing" << std::endl;
		}
		
	}
	void SocketChannel::updateEvent()
	{ 
		pLoop->updateChannel(this); 
	}
}