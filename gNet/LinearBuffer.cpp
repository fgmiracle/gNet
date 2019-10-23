#include "LinearBuffer.h"

namespace gNet
{
    LinearBuffer::LinearBuffer()
        :m_Buffer(1024)
        , m_Read(0)
        , m_Write(0)
    {
        
    }


    LinearBuffer::~LinearBuffer()
    {
        
    }

    void LinearBuffer::append(const char *data, uint32_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

}