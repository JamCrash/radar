
#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>

class rd_cond;

class rd_mutex {
    friend class rd_cond;
public:
    rd_mutex() {
        pthread_mutex_init(&_lock, NULL);
    }
    ~rd_mutex() {
        pthread_mutex_destroy(&_lock);
    }
    int rd_mutex_lock() {
        return pthread_mutex_lock(&_lock);
    }
    int rd_mutex_unlock() {
        return pthread_mutex_unlock(&_lock);
    }
private:
    pthread_mutex_t _lock;
};

class rd_cond {
public:
    rd_cond() {
        pthread_cond_init(&_cond, NULL);
    }
    ~rd_cond() {
        pthread_cond_destroy(&_cond);
    }
    int rd_cond_wait(rd_mutex* mutex) {
        return pthread_cond_wait(&_cond, &(mutex->_lock));
    }
    int rd_cond_signal() {
        return pthread_cond_signal(&_cond);
    }
    int rd_cond_broadcast() {
        return pthread_cond_broadcast(&_cond);
    }
private:
    pthread_cond_t _cond;
};

#endif /* LOCKER_H */
