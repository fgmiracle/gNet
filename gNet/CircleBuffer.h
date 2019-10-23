#ifndef __GNETCIRCLEBUFFER_H__
#define __GNETCIRCLEBUFFER_H__

#include "utils/common.h"
#include "utils/NonCopyable.hpp"

namespace gNet
{
    
    struct BufferSequence
    {
        BufferSequence() : count(0)
        {
        }

        static const int16_t MAX_BUFS = 8;
#if defined(__gnu_linux__)
		iovec   buffers[MAX_BUFS];
#define IOVEC_BUF    iov_base
#define IOVEC_LEN    iov_len

#else
		WSABUF  buffers[MAX_BUFS];
#define IOVEC_BUF    buf
#define IOVEC_LEN    len

#endif
		int16_t     count;

    };

    inline int32_t roundUp2Power(int32_t size)
    {
        assert(size >= 0 && "Size underflow");

        if (0 == size)  return 0;

        int32_t    roundSize = 1;
        while (roundSize < size)
            roundSize <<= 1;

        return roundSize;
    }

    class CircleBuffer : public NonCopyable
    {
    public:
        explicit CircleBuffer(int32_t size = 0);
        CircleBuffer(char*, int32_t);

        bool    isEmpty() const { return mWritePos == mReadPos; }
        bool    isFull()  const { return ((mWritePos + 1) & (mMaxSize - 1)) == mReadPos; }

        void    initCapacity(int size);

        bool    pushData(const char* pData, int32_t nSize);
        bool    pushDataAt(const char* pData, int32_t nSize, int32_t offset = 0);

        void    getDatum(BufferSequence& buffer, int32_t maxSize = 64*1024 - 1, int32_t offset = 0);
        void    getSpace(BufferSequence& buffer, int32_t offset = 0);

        bool    peekData(char* pBuf, int32_t nSize);
        bool    peekDataAt(char* pBuf, int32_t nSize, int32_t offset = 0);

        void    hasRead(int32_t size);
        void    hasWritten(int32_t size);

        int32_t    readableBytes()  const { return (mWritePos - mReadPos) & (mMaxSize - 1); }
        int32_t    writableBytes() const { return mMaxSize - readableBytes(); }

        int32_t        capacity() const { return mMaxSize; }

        char* beginWrite() { return begin() + mWritePos; }
        const char* beginWrite() const { return begin() + mWritePos; }

        char* beginRead() { return begin() + mReadPos; }
        const char* beginRead() const { return begin() + mReadPos; }

    private:
        char* begin() { return &*mBuffer.begin(); }
        const char* begin() const { return &*mBuffer.begin(); }

    private:
        int32_t                 mMaxSize;
        std::atomic<int32_t>    mReadPos;
        std::atomic<int32_t>    mWritePos;
        std::vector<char>       mBuffer;

    };

}


#endif // !__GNETCIRCLEBUFFER_H__
