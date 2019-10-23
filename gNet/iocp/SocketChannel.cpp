#include "SocketChannel.h"
#include "Iocp.h"

namespace gNet {

	SocketChannel::SocketChannel(EventLoop *pLoop, gNetfd sockfd, const std::string& nameArg,
		const std::string& ip, const uint32_t& port,const uint32_t& session)
        :Channel(pLoop,sockfd, nameArg, ip, port, session)
		, mIsPostRecv(false)
		, mIsPostSend(false)
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
		if (!isWriting())
		{
			nSend = ::send(sockFD, static_cast<const char*>(data), len, 0);
			if (nSend <= 0 )
			{
				if (Socket::socketErr() != WSA_IO_PENDING)
				{
					std::cout << "send error = " << Socket::socketErr() << " ." << std::endl;
					doClose(false);
					return;
				}
			}
		}
		
		uint32_t remaining = len - nSend;
		if (remaining > 0)
		{
			mSendBuf.append(static_cast<const char*>(data) + nSend, remaining);
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
		if (bCheck) 
		{
			reqCount--;
		}

		if (mStatus == kDisconnected)
		{
			return;
		}

		mStatus = kDisconnecting;
		assert(reqCount >= 0);
		if (reqCount == 0)
		{
			if (mIsPostRecv)
			{
				CancelIoEx(HANDLE(sockFD), &mOverLappeRecv.mOverLapped);
			}
			if (mIsPostSend)
			{
				CancelIoEx(HANDLE(sockFD), &mOverLappeSend.mOverLapped);
			}
			if (pLoop != nullptr)
			{
				pLoop->detach(this);
				pLoop = nullptr;
			}

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
		if (mStatus != kConnected)
		{
			return;
		}

		static WSABUF wsendbuf[1] = { { NULL, 0 } };

        DWORD dwThancferred = 0;
        int32_t nResultCode = WSASend(sockFD, wsendbuf,1, &dwThancferred, 0, &mOverLappeSend.mOverLapped, NULL);

        if ((nResultCode == SOCKET_ERROR) && (Socket::socketErr() != WSA_IO_PENDING))
        {
            std::cout << "post WSASend error = " << errno << " ." << std::endl;
            doClose(false);
            return;
        }
		
		reqCount++;
		mIsPostSend = true;      
    }

	void SocketChannel::doRecv()
    {
		if (mStatus != kConnected)
		{
			return;
		}

		static CHAR temp[] = { 0 };
		static WSABUF  in_buf = { 0, temp };

        DWORD dwThancferred = 0, dwFlags = 0;
        int32_t nResultCode = WSARecv(sockFD, &in_buf, 1, &dwThancferred, &dwFlags, &mOverLappeRecv.mOverLapped, NULL);

        if ((nResultCode == SOCKET_ERROR) && (Socket::socketErr() != WSA_IO_PENDING))
        {
            //std::cout << "post WSARecv error = " << errno << " , and close it." << std::endl << std::flush;
			doClose(false);
            return;
        }
	
		reqCount++;
		mIsPostRecv = true;      
    }

	void SocketChannel::canRecv()
	{
		if (mIsPostRecv)
		{
			reqCount--;
			mIsPostRecv = false;
			assert(reqCount >= 0);
		}

		char extrabuf[65536];
		struct iovec vec[2];
		const size_t writable = mRecvBuf.writableBytes();
		vec[0].iov_base = mRecvBuf.beginWrite();
		vec[0].iov_len = writable;
		vec[1].iov_base = extrabuf;
		vec[1].iov_len = sizeof(extrabuf);

		const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
		const int32_t nRecv = Socket::readv(sockFD, vec, iovcnt);

		if (nRecv <= 0)
		{
			//std::cout << "readv fail.error:" << errno << std::endl << std::flush;
			doClose(false);
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

		doRecv();
	}

	void SocketChannel::canSend()
	{
		disableWriting();
		
		if (mIsPostSend)
		{
			reqCount--;
			mIsPostSend = false;
			assert(reqCount >= 0);
		}

		int32_t nSend = ::send(sockFD, mSendBuf.beginRead(), mSendBuf.readableBytes(), 0);
		if (nSend <= 0)
		{
			if (Socket::socketErr() != WSA_IO_PENDING)
			{
				std::cout << "send error = " << Socket::socketErr() << " ." << std::endl;
				doClose(false);
				return;
			}
		}
		else
		{
			mSendBuf.hasRead(nSend);
			if (mSendBuf.readableBytes() > 0)
			{
				doSend();
			}
		}
	}

}