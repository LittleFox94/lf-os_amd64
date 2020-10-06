#include <pthread.h>
#include <errno.h>

#include <kernel/syscalls.h>

int pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    // XXX: do something with the attributes

    uint64_t e = 0;
    sc_do_locking_create_mutex(mutex, &e);
    return e;
}

int pthread_mutex_destroy (pthread_mutex_t *mutex) {
    uint64_t e = 0;
    sc_do_locking_destroy_mutex(*mutex, &e);
    return e;
}

int __pthread_ensure_mutex(pthread_mutex_t* mutex) {
    if(!*mutex) {
        return pthread_mutex_init(mutex, 0);
    }

    return 0;
}

int pthread_mutex_lock (pthread_mutex_t *mutex) {
    uint64_t e = 0;
    if((e = __pthread_ensure_mutex(mutex))) {
        return e;
    }

    e = 0;
    sc_do_locking_lock_mutex(*mutex, 0, &e);
    return e;
}

int pthread_mutex_trylock (pthread_mutex_t *mutex) {
    uint64_t e = 0;
    if((e = __pthread_ensure_mutex(mutex))) {
        return e;
    }

    e = 0;
    sc_do_locking_lock_mutex(*mutex, 1, &e);
    return e;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex) {
    uint64_t e = 0;
    sc_do_locking_unlock_mutex(*mutex, &e);
    return e;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    attr->type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if(
        type != PTHREAD_MUTEX_DEFAULT &&
        type != PTHREAD_MUTEX_RECURSIVE
    ) {
        return EINVAL;
    }

    attr->type = type;

    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int* type) {
    *type = attr->type;
    return 0;
}
