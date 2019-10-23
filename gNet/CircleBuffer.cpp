#include "CircleBuffer.h"

namespace gNet
{
	CircleBuffer::CircleBuffer(int32_t size)
		:mMaxSize(size)
		, mReadPos(0)
		, mWritePos(0)
	{
		mBuffer.resize(mMaxSize);
	}

	CircleBuffer::CircleBuffer(char*, int32_t)
	{

	}

	void CircleBuffer::initCapacity(int size)
	{
		mMaxSize = roundUp2Power(size);
		mBuffer.resize(mMaxSize);
		std::vector<char>(mBuffer).swap(mBuffer);
	}

	bool CircleBuffer::pushData(const char* pData, int32_t nSize)
	{
		if (!pushDataAt(pData, nSize))
			return false;

		hasWritten(nSize);
		return true;
	}

	bool CircleBuffer::pushDataAt(const char* pData, int32_t nSize, int32_t offset )
	{
		if (!pData || 0 >= nSize)
			return true;

		if (offset + nSize + 1 > writableBytes())
			return false;

		const int32_t readPos = mReadPos;
		const int32_t writePos = (mWritePos + offset) & (mMaxSize - 1);
		if (readPos > writePos)
		{
			assert(readPos - writePos > nSize);
			std::copy(pData, pData + nSize, beginWrite());
		}
		else
		{
			int32_t availBytes1 = mMaxSize - writePos;
			int32_t availBytes2 = readPos - 0;
			assert(availBytes1 + availBytes2 >= 1 + nSize);

			if (availBytes1 >= nSize + 1)
			{
				std::copy(pData, pData + nSize, beginWrite());
			}
			else
			{
				std::copy(pData, pData + availBytes1, beginWrite());
				if (nSize - availBytes1 > 0)
					std::copy(pData + availBytes1, pData + nSize, begin());
			}
		}

		return  true;
	}

	void CircleBuffer::getDatum(BufferSequence& buffer, int32_t maxSize, int32_t offset)
	{
		if (offset < 0 ||
			maxSize <= 0 ||
			offset >= readableBytes() || isEmpty())
		{
			buffer.count = 0;
			return;
		}

		//std::cout << "getDatum,mReadPos:" << mReadPos << "mWritePos:"<< mWritePos << std::endl;

		assert(mReadPos >= 0 && mReadPos  < mMaxSize);
		assert(mWritePos >= 0 && mWritePos < mMaxSize);

		int32_t   bufferIndex = 0;
		const int32_t readPos = (mReadPos + offset) & (mMaxSize - 1);
		const int32_t writePos = mWritePos;
		assert(readPos != writePos);

		buffer.buffers[bufferIndex].iov_base = beginRead();
		if (readPos < writePos)
		{
			if (maxSize < writePos - readPos)
				buffer.buffers[bufferIndex].iov_len = maxSize;
			else
				buffer.buffers[bufferIndex].iov_len = writePos - readPos;
		}
		else
		{
			int32_t nLeft = maxSize;
			if (nLeft >(mMaxSize - readPos))
				nLeft = (mMaxSize - readPos);
			buffer.buffers[bufferIndex].iov_len = nLeft;
			nLeft = maxSize - nLeft;

			if (nLeft > 0 && writePos > 0)
			{
				if (nLeft > writePos)
					nLeft = writePos;

				++bufferIndex;
				buffer.buffers[bufferIndex].iov_base = begin();
				buffer.buffers[bufferIndex].iov_len = nLeft;
			}
		}

		buffer.count = bufferIndex + 1;
	}

	void CircleBuffer::getSpace(BufferSequence& buffer, int32_t offset )
	{
		if (mMaxSize <= 0)
			return;

		assert(mReadPos >= 0 && mReadPos < mMaxSize);
		assert(mWritePos >= 0 && mWritePos < mMaxSize);

		if (writableBytes() <= offset + 1)
		{
			return;
		}

		int bufferIndex = 0;
		const int readPos = mReadPos;
		const int writePos = (mWritePos + offset) & (mMaxSize - 1);

		buffer.buffers[bufferIndex].iov_base = beginWrite();

		if (readPos > writePos)
		{
			buffer.buffers[bufferIndex].iov_len = readPos - writePos - 1;
			assert(buffer.buffers[bufferIndex].iov_len > 0);
		}
		else
		{
			buffer.buffers[bufferIndex].iov_len = mMaxSize - writePos;
			if (0 == readPos)
			{
				buffer.buffers[bufferIndex].iov_len -= 1;
			}
			else if (readPos > 1)
			{
				++bufferIndex;
				buffer.buffers[bufferIndex].iov_base = begin();
				buffer.buffers[bufferIndex].iov_len = readPos - 1;
			}
		}

		buffer.count = bufferIndex + 1;
	}

	bool CircleBuffer::peekData(char* pBuf, int32_t nSize)
	{
		if (peekDataAt(pBuf, nSize))
			hasRead(nSize);
		else
			return false;

		return true;
	}

	bool CircleBuffer::peekDataAt(char* pBuf, int32_t nSize, int32_t offset )
	{
		if (!pBuf || 0 >= nSize)
			return true;

		if (nSize + offset > readableBytes())
			return false;

		const int writePos = mWritePos;
		const int readPos = (mReadPos + offset) & (mMaxSize - 1);
		if (readPos < writePos)
		{
			assert(writePos - readPos >= nSize);
			std::copy(beginRead(), beginRead() + nSize, pBuf);
		}
		else
		{
			assert(readPos > writePos);
			int availBytes1 = mMaxSize - readPos;
			int availBytes2 = writePos - 0;
			assert(availBytes1 + availBytes2 >= nSize);

			if (availBytes1 >= nSize)
			{
				std::copy(beginRead(), beginRead() + nSize, pBuf);

			}
			else
			{
				std::copy(beginRead(), beginRead() + availBytes1, pBuf);
				assert(nSize - availBytes1 > 0);
				std::copy(begin(), begin() + nSize - availBytes1, pBuf + availBytes1);
			}
		}

		return true;
	}

	void CircleBuffer::hasRead(int32_t size)
	{
		mReadPos += size;
		mReadPos &= (mMaxSize - 1);

		//std::cout << "hasRead, mWritePos:" << mWritePos << ",mReadPos" << mReadPos << std::endl;
	}

	void CircleBuffer::hasWritten(int32_t size)
	{
		mWritePos += size;
		mWritePos &= (mMaxSize - 1);
		//std::cout << "hasWritten, mWritePos:" << mWritePos << ",mReadPos"<< mReadPos << std::endl;
	}
}