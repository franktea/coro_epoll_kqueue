#pragma once

#include <memory>
#include <coroutine>

#include "task.h"

class Send;
class Recv;
class Accept;

class Socket;
class IoContext;

class Socket {
public:
    Socket(std::string_view port, IoContext& io_context);
    
    Socket(const Socket&) = delete;
    Socket(Socket&& socket) :
        io_context_(socket.io_context_),
        fd_(socket.fd_),
        io_state_(socket.io_state_){
        socket.fd_ = -1;
    }

    ~Socket();

    task<std::shared_ptr<Socket>> accept();

    Recv recv(void* buffer, std::size_t len);

    Send send(void* buffer, std::size_t len);

    bool ResumeRecv();

    bool ResumeSend();
private:
    friend Accept;
    friend Recv;
    friend Send;
    friend IoContext;
    
    explicit Socket(int fd, IoContext& io_context);
private:
    IoContext& io_context_;
    int fd_ = -1;
    int32_t io_state_ = 0; // 当前已经注册的可读可写等事件，epoll需要用modify所以需要将旧的事件保存起来
    // 因为可能有两个协程同时在等待一个socket，所以要用两个coroutine_handle来保存。
    std::coroutine_handle<> coro_recv_; // 接收数据的协程
    std::coroutine_handle<> coro_send_; // 发送数据的协程
};
