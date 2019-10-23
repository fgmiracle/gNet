
set(SRCS
    gNet.cpp
    TcpServer.cpp
    TcpClient.cpp
    Socket.cpp
    SessionID.cpp
    LinearBuffer.cpp
    EventLoopThreadPool.cpp
    CurrentThread.cpp
    CircleBuffer.cpp
    utils/ByteOrder.cpp
 )

if(WIN32)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/iocp/Connector.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/iocp/Iocp.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/iocp/Listener.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/iocp/SocketChannel.cpp)
  
elseif(UNIX)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/epoll/Connector.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/epoll/Epoll.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/epoll/Listener.cpp)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/gNet/epoll/SocketChannel.cpp)
endif()


add_library(gNet SHARED ${SRCS})
find_package(Threads REQUIRED)
target_link_libraries(gNet pthread)

install(TARGETS gNet DESTINATION lib)

