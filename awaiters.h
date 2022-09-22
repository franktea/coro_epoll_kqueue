#pragma once

#include <type_traits>
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include "task.h"
#include "socket.h"

template<typename Syscall, typename ReturnValue>
class AsyncSyscall {
public:
    AsyncSyscall() : suspended_(false) {}

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) noexcept {
        static_assert(std::is_base_of_v<AsyncSyscall, Syscall>);
        handle_ = h;
        value_ = static_cast<Syscall*>(this)->Syscall();
        suspended_ = value_ == -1 && (errno == EAGAIN || errno == EWOULDBLOCK);
        if(suspended_) {
            // 设置每个操作的coroutine handle，recv/send在适当的epoll事件发生后才能正常调用
            static_cast<Syscall*>(this)->SetCoroHandle();
        }
        return suspended_;
    }

    ReturnValue await_resume() noexcept {
        std::cout<<"await_resume\n";
        if(suspended_) {
            value_ = static_cast<Syscall*>(this)->Syscall();
        }
        return value_;
    }
protected:
    bool suspended_;
    // 当前awaiter所在协程的handle，需要设置给socket的coro_recv_或是coro_send_来读写数据
    // handle_不是在构造函数中设置的，所以在子类的构造函数中也无法获取，必须在await_suspend以后才能设置
    std::coroutine_handle<> handle_;
    ReturnValue value_;
};

class Socket;

class Accept : public AsyncSyscall<Accept, int> {
public:
    Accept(Socket* socket) : AsyncSyscall{}, socket_(socket) {
        socket_->io_context_.WatchRead(socket_);
        std::cout<<" socket accept opertion\n";
    }

    ~Accept() {
        socket_->io_context_.UnwatchRead(socket_);
        std::cout<<"~socket accept operation\n";
    }

    int Syscall() {
        struct sockaddr_storage addr;
        socklen_t addr_size = sizeof(addr);
        std::cout<<"accept "<<socket_->fd_<<"\n";
        return ::accept(socket_->fd_, (struct sockaddr*)&addr, &addr_size);
    }

    void SetCoroHandle() {
        socket_->coro_recv_ = handle_;
    }
private:
    Socket* socket_;
    void* buffer_;
    std::size_t len_;
};

class Send : public AsyncSyscall<Send, ssize_t> {
public:
    Send(Socket* socket, void* buffer, std::size_t len) : AsyncSyscall(),
        socket_(socket), buffer_(buffer), len_(len) {
        socket_->io_context_.WatchWrite(socket_);
        std::cout<<"socket send operation\n";
    }
    ~Send() {
        socket_->io_context_.UnwatchWrite(socket_);
        std::cout<<"~ socket send operation\n";
    }

    ssize_t Syscall() {
        std::cout<<"send"<<socket_->fd_<<"\n";
        return ::send(socket_->fd_, buffer_, len_, 0);
    }

    void SetCoroHandle() {
        socket_->coro_send_ = handle_;
    }
private:
    Socket* socket_;
    void* buffer_;
    std::size_t len_;
};

class Recv : public AsyncSyscall<Recv, int> {
public:
    Recv(Socket* socket, void* buffer, size_t len): AsyncSyscall(), 
        socket_(socket), buffer_(buffer), len_(len) {
        socket_->io_context_.WatchRead(socket_);
        std::cout<<"socket recv operation\n";
    }

    ~Recv() {
        socket_->io_context_.UnwatchRead(socket_);
        std::cout<<"~socket recv operation\n";
    }

    ssize_t Syscall() {
        std::cout<<"recv fd="<<socket_->fd_<<"\n";
        return ::recv(socket_->fd_, buffer_, len_, 0);
    }
    
    void SetCoroHandle() {
        socket_->coro_recv_ = handle_;
    }
private:
    Socket* socket_;
    void* buffer_;
    std::size_t len_;
};



