#ifndef _MUTEX_H_INCLUDED
#define _MUTEX_H_INCLUDED

#include <stdint.h>

typedef uint64_t mutex_t;

#include <scheduler.h>

void init_mutex(void);

mutex_t mutex_create(void);
void mutex_destroy(mutex_t mutex);

bool mutex_lock(mutex_t mutex, pid_t holder);
bool mutex_unlock(mutex_t mutex, pid_t holder);
void mutex_unlock_holder(pid_t holder);

#endif
