#ifndef __GNET_BYTEORDER_H__
#define __GNET_BYTEORDER_H__

#include <cstdint>

namespace gNet
{
	namespace ByteOrder
	{

#define BigLittleSwap16(A) ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))

#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | (((uint32_t)(A) & 0x00ff0000) >> 8) | (((uint32_t)(A) & 0x0000ff00) << 8) | (((uint32_t)(A) & 0x000000ff) << 24))

		//host is Little-Endian, nework is Big-Endian
		// Big return 1£¬Little return 0
		bool checkCPUendian();

		uint32_t hostToNetwork32(uint32_t h);
		uint32_t networkToHost32(uint32_t n);		
		uint16_t hostToNetwork16(uint16_t h);
		uint16_t networkToHost16(uint16_t n);
		
	}
}



#endif // !__BYTEORDER_H__
