#include <pthread.h>
#include <errno.h>
#include <search.h>
#include <limits.h>
#include <stdio.h>

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*)) {
    if(hcreate_r(ULONG_MAX, (struct hsearch_data*)key)) {
        return 0;
    }

    return errno;
}

void* pthread_getspecific(pthread_key_t key) {
    char tid[20];
    snprintf(tid, 20, "%u", pthread_self());
    printf("get: %s\n", tid);

    ENTRY* e;
    if(hsearch_r((ENTRY){tid, NULL}, FIND, &e, (struct hsearch_data*)key)) {
        return e->data;
    }

    return 0;
}

int pthread_setspecific(pthread_key_t key, const void* data) {
    char tid[20];
    snprintf(tid, 20, "%u", pthread_self());
    printf("set: %s\n", tid);

    ENTRY* e;
    if(hsearch_r((ENTRY){tid, data}, ENTER, &e, (struct hsearch_data*)key)) {
        return 0;
    }
    else {
        return errno;
    }
}
