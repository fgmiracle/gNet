
#include "ByteOrder.h"

namespace gNet
{
	namespace ByteOrder
	{
		bool checkCPUendian()
		{
			static union { 
				char c[4]; 
				unsigned long l;
			} endian_test = { { 'l', '?', '?', 'b' } };

			return ((char)endian_test.l == 'b');
		}

		uint32_t hostToNetwork32(uint32_t h)
		{
			return checkCPUendian() ? h : BigLittleSwap32(h);
		}

		uint32_t networkToHost32(uint32_t n)
		{
			return checkCPUendian() ? n : BigLittleSwap32(n);
		}

		uint16_t hostToNetwork16(uint16_t h)
		{
			return checkCPUendian() ? h : BigLittleSwap16(h);
		}

		uint16_t networkToHost16(uint16_t n)
		{
			return checkCPUendian() ? n : BigLittleSwap16(n);
		}
	}
}