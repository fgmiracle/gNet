#ifndef __GNET_COMMON_H__
#define __GNET_COMMON_H__


#define SOCKET_INVALID (-1) 

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32")
#else
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#endif

#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <utility>

#include <memory.h>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "concurrentqueue/concurrentqueue.h"
#pragma GCC diagnostic pop
#else
#include "gNet/3rdparty/concurrentqueue/concurrentqueue.h"
#endif

#ifdef _WIN32
    typedef SOCKET gNetfd;
    #define iovec       _WSABUF
    #define iov_base    buf
    #define iov_len     len
	#define errno		::WSAGetLastError()
#else
    typedef int32_t gNetfd;
#endif

#include "NonCopyable.hpp"
#include "ByteOrder.h"
#include "ThreadLock.hpp"
#include "Singleton.hpp"

#define LOOPTIMEOUTMS	100
//const static int32_t sDefaultLoopTimeOutMS = 100;
#define UNUSE(x) (void)(x)

#endif