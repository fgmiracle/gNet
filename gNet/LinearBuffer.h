#ifndef __GNET_BUFFER_H__
#define __GNET_BUFFER_H__

#include "utils/ByteOrder.h"
#include "utils/NonCopyable.hpp"
#include "utils/common.h"

#define TEMBUFFER_LEN 8192

namespace gNet
{
    class LinearBuffer : public NonCopyable
    {
    public:
        LinearBuffer();
        ~LinearBuffer();
    public:
        void swap(LinearBuffer& rhs)
        {
            m_Buffer.swap(rhs.m_Buffer);
            std::swap(m_Write, rhs.m_Write);
            std::swap(m_Read, rhs.m_Read);
        }

        void append(const char *data, uint32_t len);

    
    public:

        //////////
        void reset()
        {
            m_Write = 0;
            m_Read = 0;
        }

        // data bytes
        size_t readableBytes() const
        {
            return m_Write - m_Read;;
        }

        size_t writableBytes() const
        {
            return m_Buffer.size() - m_Write; ;
        }

        // data 
        char* beginWrite()
        {
            return begin() + m_Write;
        }

        const char* beginWrite() const
        {
            return begin() + m_Write;
        }

        void hasWritten(uint32_t len)
        {
            assert(len <= writableBytes());
            m_Write += len;
        }

        //
        char* beginRead()
        {
            return begin() + m_Read;
        }

        const char* beginRead() const
        {
            return begin() + m_Read;
        }

        void hasRead(uint32_t len)
        {
            assert(len <= readableBytes());
            m_Read += len;
        }

        //
        size_t prependableBytes() const
        {
            return m_Read;
        }

        void retrieve(uint32_t len)
        {
            assert(len <= readableBytes());
            if (len < readableBytes())
            {
                m_Read += len;
            }
            else
            {
                m_Read = 0;
            }
        }

        ////
        int32_t readInt32()
        {
            assert(readableBytes() >= sizeof(int32_t));
            int32_t be32 = 0;
            ::memcpy(&be32, beginRead(), sizeof be32);
            return BigLittleSwap32(be32);
        }

        int16_t readInt16()
        {
            assert(readableBytes() >= sizeof(int16_t));
            int16_t be16 = 0;
            ::memcpy(&be16, beginRead(), sizeof be16);
            return BigLittleSwap16(be16);
        }

        uint16_t readUint16()
        {
            assert(readableBytes() >= sizeof(uint16_t));
            uint16_t be16 = 0;
            ::memcpy(&be16, beginRead(), sizeof be16);
            return BigLittleSwap16(be16);
        }

        int8_t readInt8()
        {
            int8_t result = *beginRead();
            return result;
        }
        
        //
        void ensureWritableBytes(uint32_t len)
        {
            if (writableBytes() < len)
            {
                makeSpace(len);
            }

            assert(writableBytes() >= len);
        }
        

    private:
        char* begin()
        {
            return &*m_Buffer.begin();
        }

        const char* begin() const
        {
            return &*m_Buffer.begin();
        }

        bool makeSpace(uint32_t len)
        {
            if (writableBytes() + prependableBytes() < len)
            {                
                m_Buffer.resize(m_Write + len);                
            }
            else
            {
                // move readable data to the front, make space inside buffer
                size_t readable = readableBytes();
                std::copy(begin() + m_Read, begin() + m_Write, begin());
                m_Read = 0;
                m_Write = m_Read + readable;
                return true;
            }

            return false;
        }

    protected:
        std::vector<char>        m_Buffer;
        size_t                    m_Read;
        size_t                    m_Write;
    };

}

#endif // !__BUFFER_H__