#include "io_context.h"
#include "awaiters.h"

task<bool> inside_loop(Socket& socket) {
    char buffer[1024] = {0};
    ssize_t recv_len = co_await socket.recv(buffer, sizeof(buffer));
    ssize_t send_len = 0;
    while(send_len < recv_len) {
        ssize_t res = co_await socket.send(buffer + send_len, recv_len - send_len);
        if(res <= 0) {
            co_return false;
        }
        send_len += res;
    }

    std::cout<<"Done send "<<send_len<<"\n";
    if(recv_len <= 0) {
        co_return false;
    }
    printf("%s\n", buffer);
    co_return true;
}

task<> echo_socket(std::shared_ptr<Socket> socket) {
    for(;;) {
        std::cout<<"BEGIN\n";
        bool b = co_await inside_loop(*socket);
        if(!b) break;
        std::cout<<"END\n";
    }
}

task<> accept(Socket& listen) {
    for(;;) {
        auto socket = co_await listen.accept();
        auto t = echo_socket(socket);
        t.resume();
    }
}

int main() {
    IoContext io_context;
    Socket listen{"10009", io_context};
    auto t = accept(listen);
    t.resume();

    io_context.run(); // 启动事件循环
}
