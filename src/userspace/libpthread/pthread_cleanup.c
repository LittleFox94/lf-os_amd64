#include <pthread.h>

static __thread struct _pthread_cleanup_context *_pthread_cleanup_stack = 0;

void _pthread_cleanup_push(struct _pthread_cleanup_context *context, void (*routine)(void*), void *arg) {
    context->_routine  = routine;
    context->_arg      = arg;
    context->_previous = _pthread_cleanup_stack;
    _pthread_cleanup_stack = context;
}

void _pthread_cleanup_pop(struct _pthread_cleanup_context *context, int execute) {
    if(execute) {
        context->_routine(context->_arg);
    }

    _pthread_cleanup_stack = context->_previous;
}
