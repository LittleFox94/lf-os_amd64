#define _POSIX_THREADS
#include <pthread.h>

#include <kernel/syscalls.h>

int pthread_cond_init (pthread_cond_t *condvar, const pthread_condattr_t *attributes) {
    // XXX: do something with the attributes

    uint64_t e = 0;
    sc_do_locking_create_condvar(condvar, &e);
    return e;
}

int pthread_cond_destroy (pthread_cond_t *condvar) {
    uint64_t e = 0;
    sc_do_locking_destroy_condvar((uint64_t)condvar, &e);
    return e;
}

int pthread_cond_signal (pthread_cond_t *condvar) {
    uint64_t e = 0;
    sc_do_locking_signal_condvar((uint64_t)condvar, 1, &e); // XXX: check if amount = 1 is correct with POSIX.1c
    return e;
}

int pthread_cond_broadcast (pthread_cond_t *condvar) {
    uint64_t e = 0;
    sc_do_locking_signal_condvar((uint64_t)condvar, 0, &e);
    return e;
}

int pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex) {
    uint64_t e = 0;

    if((e = pthread_mutex_unlock(mutex))) {
        return e;
    }

    sc_do_locking_wait_condvar((uint64_t)cond, 0, &e);
    if(e) {
        return e;
    }

    return pthread_mutex_lock(mutex);
}

int pthread_cond_timedwait (pthread_cond_t *__cond, pthread_mutex_t *__mutex, const struct timespec *__abstime) {
    return 95; // XXX: errno.h ... this one is ENOTSUP
}
