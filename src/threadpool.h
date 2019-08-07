
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <list>
#include "locker.h"
#include "dbg.h"

#define THREADNUM 8

class task_t {
    typedef void (*Func)(void*);
public:
    task_t() = delete;
    task_t(Func f, void* argument): func(f), arg(argument) {}
    void do_it() const { func(arg); }
    ~task_t() = default;
private:
    Func func;
    void* arg;
};

class threadpool {
    typedef void (*Func)(void*);
public:
    threadpool(int thn);
    ~threadpool();
    int threadpool_destroy();
    int add_task(Func f, void* arg);
private:
    rd_mutex lock;
    rd_cond  cond;
    int thread_num;
    int shutdown;
    std::vector<pthread_t*> threads;
    std::list<task_t*> tasks;
    static void* thread_worker(void*);
    void threadpool_free();
};

#endif /* THREADPOOL_H */
