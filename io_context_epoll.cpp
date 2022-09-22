#include <stdexcept>
#include <sys/epoll.h>
#include <cstring>
#include "io_context.h"
#include "socket.h"

IoContext::IoContext(): fd_(epoll_create1(0)) {
    if(fd_ == -1) {
        throw std::runtime_error{"epoll_create1"};
    }
}

void IoContext::run() {
    struct epoll_event ev, events[max_events];
    for(;;) {
        int nfds = epoll_wait(fd_, events, max_events, -1);
        if(nfds == -1) {
            throw std::runtime_error{"epoll_wait"};
        }

        for(int i = 0; i < nfds; ++i) {
            auto socket = static_cast<Socket*>(events[i].data.ptr);
            if(events[i].events & EPOLLIN) {
                socket->ResumeRecv();
            }
            if(events[i].events & EPOLLOUT) {
                socket->ResumeSend();
            }
        }
    }
}

void IoContext::Attach(Socket* socket) {
    struct epoll_event ev;
    auto io_state = EPOLLIN | EPOLLET;
    ev.events = io_state;
    ev.data.ptr = socket;
    if(epoll_ctl(fd_, EPOLL_CTL_ADD, socket->fd_, &ev) == -1) {
        throw std::runtime_error{"epoll_ctl: attach"};
    }
    socket->io_state_ = io_state;
}

// 定义一个宏，消除重复的代码
#define UpdateState(new_state) \
    if(socket->io_state_ != new_state) { \
        struct epoll_event ev = {}; \
        ev.events = new_state; \
        ev.data.ptr = socket; \
        if(epoll_ctl(fd_, EPOLL_CTL_MOD, socket->fd_, &ev) == -1) { \
            throw std::runtime_error{"epoll_ctl: mod"}; \
        } \
        socket->io_state_ = new_state; \
    } \

void IoContext::WatchRead(Socket* socket) {
    auto new_state = socket->io_state_ | EPOLLIN;
    UpdateState(new_state);
}

void IoContext::UnwatchRead(Socket* socket) {
    auto new_state = socket->io_state_ & ~EPOLLIN;
    UpdateState(new_state);
}

void IoContext::WatchWrite(Socket* socket) {
    auto new_state = socket->io_state_ | EPOLLOUT;
    UpdateState(new_state);
}

void IoContext::UnwatchWrite(Socket* socket) {
    auto new_state = socket->io_state_ & ~EPOLLOUT;
    UpdateState(new_state);
}

void IoContext::Detach(Socket* socket) {
    if(epoll_ctl(fd_, EPOLL_CTL_DEL, socket->fd_, nullptr) == -1) {
        perror("epoll ctl: del");
        exit(EXIT_FAILURE);
    }
}
