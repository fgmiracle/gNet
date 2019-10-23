#ifndef __GNET_SOCKET_H__
#define __GNET_SOCKET_H__

#include <gNet/utils/common.h>

namespace gNet {
    namespace Socket
    {
		typedef union sockaddr_all_t {
			struct sockaddr s;
			struct sockaddr_in v4;
			struct sockaddr_in6 v6;
		}sockaddr_all;

        gNetfd createSocket(bool isIPv6 = false);
		bool fillAddr(const char *ip,uint16_t port, sockaddr_storage *addr, bool isIPv6 = false);
        bool bind(gNetfd &socket, uint16_t port,const char *ip = nullptr,bool isIPv6 = false);
		bool bindLocal(gNetfd &socket, bool isIPv6 = false);
		bool connect(gNetfd &socket, uint16_t port, const char *ip, bool isIPv6 = false);
        bool listen(gNetfd &socket);
		gNetfd acceptEx(gNetfd listenSocket, struct sockaddr* addr, socklen_t* addrLen);

        bool setNonBlock(gNetfd &socket);
        bool setNoDelay(gNetfd &socket);
        bool setReuseAddr(gNetfd &socket);
        bool setReusePort(gNetfd &socket);
        bool setIPV6Only(gNetfd &socket, bool only);
        bool setKeepAlive(gNetfd &socket);
        int32_t setSendSize(gNetfd &socket, int32_t size);
        int32_t setRecvSize(gNetfd &socket, int32_t size);

        void getAddr(sockaddr_all *sa,std::string &ip,uint16_t &port);
		void getName(sockaddr_all *addr, std::string &ip, unsigned short *port);

        void closeSocket(gNetfd &socket);
		int32_t shutDown(gNetfd &socket);
		int32_t socketErr();
#ifdef _WIN32
		int32_t readv(gNetfd &socket, struct iovec* iov, int32_t iovcnt);
#endif
    }
}

#endif