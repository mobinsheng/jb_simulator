#ifndef SIMPLE_LOCK_H
#define SIMPLE_LOCK_H

#include <pthread.h>

class SimpleLock
{
public:
    SimpleLock(){
        pthread_mutex_init(&lock,NULL);
    }

    void Lock(){
        pthread_mutex_lock(&lock);
    }

    void Unlock(){
        pthread_mutex_unlock(&lock);
    }

    ~SimpleLock(){
        pthread_mutex_destroy(&lock);
    }


private:
    pthread_mutex_t lock;
};

#endif // SIMPLE_LOCK_H
