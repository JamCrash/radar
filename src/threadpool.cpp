
#include "dbg.h"
#include "threadpool.h"

threadpool::threadpool(int thn): shutdown(0)
{
    if(thn <= 0)
        thn = THREADNUM;
    thread_num = thn;

    threads.resize(thread_num, nullptr);
    for(int i = 0; i < thread_num; ++i) {
        threads[i] = new pthread_t;
        if(threads[i] == nullptr) {
            log_err("pthread alloc error");
            threadpool_free();
        }
    }

    int err;
    for(int i = 0; i < thread_num; ++i) {
        err = pthread_create(threads[i], NULL, thread_worker, this);
        if(err != 0) {
            log_err("pthread %ld create error", threads[i]);
            threadpool_destroy();
        }
    }
}

int
threadpool::add_task(Func f, void* arg) 
{
    int err = 0;

    if(f == nullptr || arg == nullptr) {
        log_err("add_task error(arguments can not be nullptr)");
        return -1;
    }

    task_t* new_task = new task_t(f, arg);
    if(new_task == nullptr) {
        log_err("alloc new task error");
        return -1;
    }

    lock.rd_mutex_lock();

    tasks.push_back(new_task);

    lock.rd_mutex_unlock();
    cond.rd_cond_signal();
    
    log_info("add new task succeed");
    return err;
}

void
threadpool::threadpool_free() noexcept
{
    for(int i = 0; i < thread_num; ++i) {
        if(threads[i] != nullptr) {
            delete threads[i];
        }
    }

    for(auto& task: tasks) {
        if(task != nullptr) {
            delete task;
        }
    }
}

int
threadpool::threadpool_destroy()
{
    int err = 0;
    do{
        if(shutdown) {
            log_err("threadpool already shutdown");
            break;
        }

        err = cond.rd_cond_broadcast();
        if(err != 0) {
            log_err("send broadcast signal failed");
            break;
        }

        shutdown = 1;

        for(int i = 0; i < thread_num; ++i) {
            if(threads[i] != nullptr) {
                pthread_join(*threads[i], NULL);    //TODO: join failed
                log_info("thread %ld join succeed", *threads[i]);
            }
        }
        log_info("all threads join succeed");
    }while(0);

    threadpool_free();

    return err;
}

void*
threadpool::thread_worker(void* arg)
{
    threadpool* pool = static_cast<threadpool*>(arg);

    while(1) {
        pool->lock.rd_mutex_lock();
        while((pool->tasks.size() == 0) && !pool->shutdown ) {
            pool->cond.rd_cond_wait(&(pool->lock));
        }

        if(pool->shutdown != 0) {
            break;
        }

        task_t* task = pool->tasks.front();
        pool->tasks.pop_front();
        pool->lock.rd_mutex_unlock();

        task->do_it();
        delete task;
    }
    pool->lock.rd_mutex_unlock();

    log_info("thread %ld exit", pthread_self());
    pthread_exit(NULL);

    return NULL;
}