#ifndef __GNET_SOCKETCHANNEL_H__
#define __GNET_SOCKETCHANNEL_H__

#include "gNet/Channel.h"
#include "gNet/CircleBuffer.h"
#include "gNet/LinearBuffer.h"

namespace gNet {
    class SocketChannel: public Channel
    {
    public:
		SocketChannel(EventLoop *pLoop, 
			gNetfd sockfd, 
			const std::string& nameArg,
			const std::string& ip, 
			const uint32_t& port,
			const uint32_t& session);
        ~SocketChannel();
    public:
        void    sendData(const std::string &msg);
		void    sendData(const char *buf, uint32_t inLen);
		void	shutDown();
		void	connectEstablished();
		void	connectDestroyed();
		void	setNetPack(uint32_t maxLen);

		void    enableReading() { mEvents |= mReadEvent; doRecv(); }
		void    enableWriting() { mEvents |= mWriteEvent; doSend(); }

		void	disableReading() { mEvents &= ~mReadEvent; }
		void	disableWriting() { mEvents &= ~mWriteEvent;}

		bool	isWriting() const { return mEvents & mWriteEvent; }
		bool	isReading() const { return mEvents & mReadEvent; }

    private:
		void    doAccept(OverLapped *overLapped) override {}
        void    doClose(bool bCheck = true);
		void	canRecv();
		void	canSend();

		void	sendInLoopStr(const std::string &msg);
		void	sendInLoopPtr(const void* data, size_t len);

		void    shutDownInLoop();

		void    doSend();
		void    doRecv();

    private:

        // win OVERLAPPED
        OverLappedRecv          mOverLappeRecv;
        OverLappedSend          mOverLappeSend;
        // buf
        LinearBuffer            mRecvBuf;
		LinearBuffer            mSendBuf;
        //CircleBuffer            mSendBuf;
        char                    mExtrabuf[256];

		//
		bool					mIsPostRecv;
		bool					mIsPostSend;
    };
}

#endif