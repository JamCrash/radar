
#include "epoll.h"
#include "dbg.h"

struct epoll_event events[MAXEVENT];

epoll_t::epoll_t(int flag = 0)
{
    if(flag < 0) 
        flag = 0;

    epfd = epoll_create1(flag);
}

int 
epoll_t::add(int fd, struct epoll_event* event)
{
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event) != 0) {
        log_err("add epoll event failed");
        return -1;
    }
    return 0;
}

int
epoll_t::del(int fd, struct epoll_event* event)
{
    if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, event) != 0) {
        log_err("delete epoll event failed");
        return -1;
    }
    return 0;
}

int
epoll_t::mod(int fd, struct epoll_event* event)
{
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, event) != 0) {
        log_err("change epoll event failed");
        return -1;
    }
    return 0;
}

int
epoll_t::wait(struct epoll_event* events, int maxevents, int timeout) 
{
    return epoll_wait(epfd, events, maxevents, timeout);
}