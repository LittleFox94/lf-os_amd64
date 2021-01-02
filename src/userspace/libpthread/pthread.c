#include <pthread.h>
#include <errno.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start)(void*), void* arg) {
    return EINVAL;
}

int pthread_cancel(pthread_t thread) {
    return EINVAL;
}

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

int pthread_setcancelstate(int state, int *oldstate) {
    return EINVAL;
}

int pthread_setcanceltype(int type, int *oldtype) {
    return EINVAL;
}

void pthread_testcancel() {
}

int pthread_attr_init(pthread_attr_t *attr) {
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    return 0;
}
