
#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

#define MAXEVENT 256

class epoll_t {
public:
    epoll_t(int flag = 0);
    ~epoll_t() = default;
    int add(int fd, struct epoll_event* event);
    int del(int fd, struct epoll_event* event);
    int mod(int fd, struct epoll_event* event);
    int wait(struct epoll_event* events, int maxevents, int timeout);
    int get_epfd() const {
        return epfd; 
    }
private:
    int epfd;
};

#endif  /* EPOLL_H */