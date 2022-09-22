#include <memory>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <string_view>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include "socket.h"
#include "io_context.h"
#include "awaiters.h"

Socket::Socket(std::string_view port, IoContext& io_context) :
    io_context_(io_context) {
    struct addrinfo hints, *res;

    std::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; // use IPv4 or v6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me 

    getaddrinfo(NULL, port.data(), &hints, &res);
    fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int opt;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(::bind(fd_, res->ai_addr, res->ai_addrlen) == -1) {
        throw std::runtime_error{"bind error"};
    }
    ::listen(fd_, 8);
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
    io_context_.WatchRead(this);
}

Socket::~Socket() {
    if(fd_ == -1) return;
    io_context_.Detach(this);
    std::cout<<"close fd="<<fd_<<"\n";
    ::close(fd_);
}

task<std::shared_ptr<Socket>> Socket::accept() {
    int fd = co_await Accept{this};
    if(fd == -1) {
        throw std::runtime_error{"accept error"};
    }
    co_return std::shared_ptr<Socket>(new Socket{fd, io_context_});
}

Recv Socket::recv(void* buffer, std::size_t len) {
    return Recv{this, buffer, len};
}

Send Socket::send(void* buffer, std::size_t len) {
    return Send{this, buffer, len};
}

bool Socket::ResumeRecv() {
    if(!coro_recv_) { std::cout<<"no handle for recv\n"; return false; }
    coro_recv_.resume();
    return true;
}

bool Socket::ResumeSend() {
    if(!coro_send_) { std::cout<<"no handle for send\n"; return false; }
    coro_send_.resume();
    return true;
}

Socket::Socket(int fd, IoContext& io_context) : io_context_(io_context),
    fd_(fd) {
    fcntl(fd_, F_SETFL, O_NONBLOCK);
    io_context_.Attach(this);
}
