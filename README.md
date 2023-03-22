# coro_epoll_kqueue
c++20 coroutine with epoll and queue
C++20 coroutine对于epoll和kqueue的封装示例，只有500行代码，适合学习参考。echo_server.cpp就是使用示例。

编译：
```
git clone https://github.com/franktea/coro_epoll_kqueue.git;
cd coro_epoll_kqueue;
mkdir -p build;
cd build;
cmake ..
make
```

最初参考了https://github.com/Ender-events/epoll-coroutine
