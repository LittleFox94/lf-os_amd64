#include <pthread.h>
#include <errno.h>

pthread_t pthread_self() {
    return getpid();
}

int pthread_equal(pthread_t a, pthread_t b) {
    return a == b;
}

int pthread_once(pthread_once_t* once_control, void(*f)(void)) {
    int e;

    if(!once_control->exec_mutex) {
        if((e = pthread_mutex_init(&once_control->exec_mutex, 0))) {
            return e;
        }
    }
    if((e = pthread_mutex_lock(&once_control->exec_mutex))) {
        return e;
    }

    if(!once_control->init_executed) {
        f();
        once_control->init_executed = 1;
    }

    if((e = pthread_mutex_unlock(&once_control->exec_mutex))) {
        return e;
    }

    return 0;
}

int pthread_join(pthread_t thread, void **retval) {
    return ESRCH;
}

int pthread_detach(pthread_t thread) {
    return ESRCH;
}
