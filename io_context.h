#pragma once

#include <set>

class Socket;
class Send;
class Recv;
class Accept;

class IoContext {
public:
    IoContext();

    void run();
private:
    constexpr static std::size_t max_events = 10;
    const int fd_;
    friend Socket;
    friend Send;
    friend Recv;
    friend Accept;
    void Attach(Socket* socket);
    void WatchRead(Socket* socket);
    void UnwatchRead(Socket* socket);
    void WatchWrite(Socket* socket);
    void UnwatchWrite(Socket* socket);
    void Detach(Socket* socket);
};
