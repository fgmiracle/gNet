#include "Socket.h"
namespace gNet {
    namespace Socket
    {
        gNetfd createSocket(bool isIPv6)
        {
            gNetfd mSocket = SOCKET_INVALID;
            if (isIPv6)
            {
#ifdef _WIN32
                mSocket = ::WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
                if (mSocket == INVALID_SOCKET)
                {
                    std::cout << "TcpSocket create error!ERRCODE = " << ::WSAGetLastError() << std::endl;
                    return SOCKET_INVALID;
                }
#else
                mSocket = ::socket(AF_INET6, SOCK_STREAM, 0);
                if (mSocket == -1)
                {
                    std::cout << "TcpSocket create error!ERRCODE = " << errno << std::endl;
                    return SOCKET_INVALID;
                }
#endif
            }
            else
            {
#ifdef _WIN32
                mSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				//mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
                if (mSocket == INVALID_SOCKET)
                {
                    std::cout << "TcpSocket create error!ERRCODE = " << ::WSAGetLastError() << std::endl;
                    return SOCKET_INVALID;
                }
#else
                mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
                if (mSocket == -1)
                {
                    std::cout << "TcpSocket create error!ERRCODE = " << errno << std::endl;
                    return SOCKET_INVALID;
                }
#endif
            }
            return mSocket;
        }

		bool fillAddr(const char *ip, uint16_t port, sockaddr_storage *outAddr, bool isIPv6)
		{
			memset(outAddr, 0, sizeof(sockaddr_storage));

			if (isIPv6)
			{
				sockaddr_in6 *in6 = (struct sockaddr_in6*)outAddr;
				in6->sin6_family = AF_INET6;
				if (nullptr == ip)
				{
					in6->sin6_addr = in6addr_any;
				}
				else
				{
					// inet_pton can handle ipv6 and ipv4
					auto ret = inet_pton(AF_INET6, ip, &(in6->sin6_addr));
					if (ret != 1)
					{
						std::cout << "bind ipv6 error, ipv6 format error" << ip << std::endl;
						return false;
					}
				}

				in6->sin6_port = htons(port);
			}
			else
			{
				sockaddr_in *in4 = (struct sockaddr_in*)outAddr;
				in4->sin_family = AF_INET;
				in4->sin_addr.s_addr = (nullptr == ip) ? INADDR_ANY : inet_addr(ip);
				in4->sin_port = htons(port);
			}

			return true;
		}

        bool bind(gNetfd &socket, uint16_t port, const char *ip, bool isIPv6)
        {
			sockaddr_storage localAddrs;
			if (!fillAddr(ip, port, &localAddrs, isIPv6))
			{
				Socket::closeSocket(socket);
				socket = SOCKET_INVALID;
				return false;
			}

			if (::bind(socket, (const sockaddr *)&localAddrs, sizeof(sockaddr_storage)) == SOCKET_INVALID)
			{
				std::cout << "bind error, ERRCODE=" << Socket::socketErr() << std::endl;
				Socket::closeSocket(socket);
				socket = SOCKET_INVALID;
				return false;
			}

            return true;
        }

		bool bindLocal(gNetfd &socket, bool isIPv6 )
		{
			sockaddr_storage localAddrs = { 0 };
			if (isIPv6)
			{
				sockaddr_in6 *in6 = (struct sockaddr_in6*)&localAddrs;
				in6->sin6_family = AF_INET6;
			}
			else 
			{
				sockaddr_in *in4 = (struct sockaddr_in*)&localAddrs;
				in4->sin_family = AF_INET;
			}

			if (::bind(socket, (const sockaddr *)&localAddrs, sizeof(sockaddr_storage)) == SOCKET_INVALID)
			{
				std::cout << "bind error, ERRCODE=" << Socket::socketErr() << std::endl;
				return false;
			}

			return true;
		}
		
		bool connect(gNetfd &socket, uint16_t port, const char *ip, bool isIPv6)
		{
			if (isIPv6)
			{
				sockaddr_in6 addr = { 0 };
				addr.sin6_family = AF_INET6;
				int32_t ret = inet_pton(AF_INET6, ip, &addr.sin6_addr);
				if (ret != 1)
				{
					std::cout << "bind ipv6 error, ipv6 format error" << ip << std::endl;
					//Socket::closeSocket(socket);
					return false;
				}

				addr.sin6_port = htons(port);
				if (connect(socket, (struct sockaddr *) &addr, sizeof(addr)) != 0)
				{
					std::cout << "ipv6 connect error!ip= " << ip << ",error=" << Socket::socketErr() << std::endl;
					//Socket::closeSocket(socket);
					return false;
				}
			}
			else
			{
				sockaddr_in addr = { 0 };
				memset(&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				int32_t ret = inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
				if (ret != 1)
				{
					std::cout << "bind ipv4 error, ipv4 format error" << ip << std::endl;
					//Socket::closeSocket(socket);
					return false;
				}

				addr.sin_port = htons(port);
				if (connect(socket, (struct sockaddr *) &addr, sizeof(addr)) != 0)
				{
					std::cout << "ipv4 connect error!ip= " << ip << ",error=" << Socket::socketErr() << std::endl;
					//Socket::closeSocket(socket);
					return false;
				}
			}

			return true;
		}

        bool listen(gNetfd &socket)
        {
#ifndef SOMAXCONN
#define SOMAXCONN 1024
#endif
            if (::listen(socket, SOMAXCONN) != 0)
            {
                std::cout <<"listen error, ERRCODE=" << Socket::socketErr()<< std::endl;
                Socket::closeSocket(socket);
                socket = SOCKET_INVALID;
                return false;
            }

            return true;
        }

		gNetfd acceptEx(gNetfd listenSocket, sockaddr * addr, socklen_t * addrLen)
		{
			return accept(listenSocket, addr, addrLen);
		}

        bool setNonBlock(gNetfd &socket)
        {
#ifdef _WIN32
            unsigned long val = 1;
            return ioctlsocket(socket, FIONBIO, &val) == NO_ERROR;
#else
            return fcntl((socket), F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK) == 0;
#endif
        }
        
        bool setNoDelay(gNetfd &socket)
        { 
            int32_t bTrue = 1;
            return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0;

        }


        bool setReuseAddr(gNetfd &socket)
        { 
            int32_t bReUseAddr = 1;            
            return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&bReUseAddr, sizeof(bReUseAddr)) == 0;

        }

        bool setReusePort(gNetfd &socket)
        {
            int32_t bReUsePort = 1;        
            return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,    (char*)&bReUsePort,(sizeof bReUsePort));

        }


        bool setIPV6Only(gNetfd &socket, bool only)
        { 
            int32_t ipv6only = only ? 1 : 0;
            return setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only)) == 0;

        }

        bool setKeepAlive(gNetfd &socket)
        {
            int32_t keepAlive = 1;
            return setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));

        }

        int32_t setSendSize(gNetfd &socket, int32_t size)
        {
            return setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
        }

        int32_t setRecvSize(gNetfd &socket, int32_t size)
        {
            return setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
        }

		void getAddr(sockaddr_all *sa, std::string &ip, uint16_t &port)
        {
			char tmp[INET6_ADDRSTRLEN] = { 0 };
			void * sin_addr = (sa->s.sa_family == AF_INET) ? (void*)&sa->v4.sin_addr : (void *)&sa->v6.sin6_addr;
			port = ntohs((sa->s.sa_family == AF_INET) ? sa->v4.sin_port : sa->v6.sin6_port);
			if (inet_ntop(sa->s.sa_family, sin_addr, tmp, sizeof(tmp))) 
			{
				ip = tmp;
			}
			
        }
		void getName(sockaddr_all * addr, std::string & ip, unsigned short * port)
		{
			char tmp[INET6_ADDRSTRLEN];
			void * sin_addr = (addr->s.sa_family == AF_INET) ? (void*)&addr->v4.sin_addr : (void *)&addr->v6.sin6_addr;
			*port = ntohs((addr->s.sa_family == AF_INET) ? addr->v4.sin_port : addr->v6.sin6_port);
			if (inet_ntop(addr->s.sa_family, sin_addr, tmp, sizeof(tmp))) {
				ip = tmp;
			}
		}

#ifdef _WIN32
		int32_t readv(gNetfd &socket, struct iovec* iov, int32_t iovcnt)
		{
			DWORD readn = 0;
			DWORD flags = 0;

			if (::WSARecv(socket, iov, iovcnt, &readn, &flags, nullptr, nullptr) == 0) 
			{
				return readn;
			}

			return -1;
		}
#endif

        int32_t socketErr()
        {
#ifdef _WIN32
            return  ::WSAGetLastError();
#else
            return errno;
#endif
        }

		int32_t shutDown(gNetfd &socket)
		{

#ifdef _WIN32
			return shutdown(socket, SD_SEND);
#else
			return shutdown(socket, SHUT_WR);
#endif

		}

        void closeSocket(gNetfd &socket)
        {
#ifdef _WIN32
            ::closesocket(socket);
#else
            ::close(socket);
#endif
            socket = SOCKET_INVALID;
        }
    }
}