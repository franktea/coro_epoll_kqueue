#include <stdexcept>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include "io_context.h"
#include "socket.h"

IoContext::IoContext(): fd_(kqueue()) {
    if(fd_ == -1) {
        throw std::runtime_error{"kqueue() error"};
    }
}

void IoContext::run() {
    struct kevent ev, events[max_events];
    for(;;) {
        int nfds = kevent(fd_, NULL, 0, events, max_events, NULL);
        if(nfds == -1) {
            throw std::runtime_error{"kevent()"};
        }

        for(int i = 0; i < nfds; ++i) {
            auto socket = static_cast<Socket*>(events[i].udata);
            if(events[i].filter == EVFILT_READ) {
                socket->ResumeRecv();
            }
            if(events[i].filter == EVFILT_WRITE) {
                socket->ResumeSend();
            }
        }
    }
}

void IoContext::Attach(Socket* socket) {
    struct kevent ev;
    auto io_state = EVFILT_READ;
    EV_SET(&ev, socket->fd_, EVFILT_READ, EV_ADD, 0, 0, socket);
    if(-1 == kevent(fd_, &ev, 1, NULL, 0, NULL)) {
        throw std::runtime_error{"kevnet: ADD"};
    }
    socket->io_state_ = io_state;
}

// 定义一个宏，消除重复的代码
#define UpdateStatus(new_state, filter, flags) \
    if(socket->io_state_ != new_state) { \
        struct kevent ev; \
        EV_SET(&ev, socket->fd_, filter, flags, 0, 0, socket); \
        if(-1 == kevent(fd_, &ev, 1, NULL, 0, NULL)) { \
            throw std::runtime_error{"kevent"}; \
        } \
        socket->io_state_ = new_state; \
    }
    
void IoContext::WatchRead(Socket* socket) {
    auto new_state = socket->io_state_ | EVFILT_READ;
    UpdateStatus(new_state, EVFILT_READ, EV_ADD);
}

void IoContext::UnwatchRead(Socket* socket) {
    auto new_state = socket->io_state_ & ~EVFILT_READ;
    UpdateStatus(new_state, EVFILT_READ, EV_DELETE);
}

void IoContext::WatchWrite(Socket* socket) {
    auto new_state = socket->io_state_ | EVFILT_WRITE;
    UpdateStatus(new_state, EVFILT_WRITE, EV_ADD);
}

void IoContext::UnwatchWrite(Socket* socket) {
    auto new_state = socket->io_state_ & ~EVFILT_WRITE;
    UpdateStatus(new_state, EVFILT_WRITE, EV_DELETE);
}

void IoContext::Detach(Socket* socket) {
    ::close(socket->fd_);
}
