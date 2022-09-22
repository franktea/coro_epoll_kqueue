cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

# set the project name
project(coro_epoll)

if(UNIX AND NOT APPLE)
    add_executable(coro_epoll echo_server.cpp io_context_epoll.cpp socket.cpp)
else()
    add_executable(coro_kqueue echo_server.cpp io_context_kqueue.cpp socket.cpp)
endif()

