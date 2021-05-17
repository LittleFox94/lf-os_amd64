#include <tpa.h>
#include <vm.h>
#include <bluescreen.h>
#include <mutex.h>
#include <log.h>
#include <scheduler.h>
#include <errno.h>

static tpa_t*   mutexes;
static uint64_t next_mutex = 1;

struct mutex_data {
    //! Current state of this mutex, 0 = unlocked
    int state;

    //! PID of the process who holds the current lock
    pid_t holder;
};

void init_mutex() {
    mutexes = tpa_new(&kernel_alloc, sizeof(struct mutex_data), 4080, 0);
}

mutex_t mutex_create() {
    if(!next_mutex) {
        panic_message("Mutex namespace overflow!");
    }

    struct mutex_data data = {
        .state      = 0,
        .holder     = 0,
    };

    tpa_set(mutexes, next_mutex, &data);
    return next_mutex++;
}

void mutex_destroy(mutex_t mutex) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    if(!data) {
        logw("mutex", "Tried to destroy non-existing mutex %u", mutex);
        return;
    }
    else if(data->state) {
        panic_message("Tried to destroy locked mutex!");
    }

    tpa_set(mutexes, mutex, 0);
}

bool mutex_lock(mutex_t mutex, pid_t holder) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    if(!data) {
        panic_message("Tried to lock non-existing mutex!");
    }
    else if(data->state && data->holder != holder) {
        return false;
    }
    else {
        ++data->state;
        data->holder = holder;
        return true;
    }
}

bool mutex_unlock(mutex_t mutex, pid_t holder) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    if(!data) {
        logw("mutex", "Tried to destroy non-existing mutex %u", mutex);
        return false;
    }
    else if(!data->state) {
        logw("mutex", "Tried to unlock unlocked mutex %u", mutex);
        return false;
    }
    else if(data->holder != holder) {
        panic_message("Tried to unlock stolen mutex");
    }
    else {
        --data->state;

        if(!data->state) {
            union wait_data wd;
            wd.mutex = mutex;
            scheduler_waitable_done(wait_reason_mutex, wd, 1);
        }

        return true;
    }
}

void mutex_unlock_holder(pid_t pid) {
    size_t prev = 0;
    size_t mutex = 1;
    do {
        logd("mutex", "Looking at the holder of %d", mutex);
        struct mutex_data* data = tpa_get(mutexes, mutex);

        // we have to check if the mutex is valid since the first
        // iteration of this loop always looks at mutex 1 which could
        // be invalid by now. Following iterations should be validity-
        // checked by the condition of the loop
        if(data && data->state && data->holder == pid) {
            logw("mutex", "Mutex %d was held by %d but we are unlocking, probably because the process is dead - mutex leak possible",
                    mutex, pid);

            mutex_unlock(mutex, pid);
        }

        prev = mutex;
    } while((mutex = tpa_next(mutexes, prev)) > prev);
}

void sc_handle_locking_create_mutex(uint64_t* mutex, uint64_t* error) {
    if(!next_mutex) {
        *error = ENOMEM;
        logw("mutex", "Mutex namespace overflow!");
        return;
    }

    *error = 0;
    *mutex = mutex_create();
}

void sc_handle_locking_destroy_mutex(uint64_t mutex, uint64_t* error) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    if(!data) {
        *error = EINVAL;
        return;
    }
    else if(data->state) {
        *error = EBUSY;
        return;
    }

    *error = 0;
    mutex_destroy(mutex);
}

void sc_handle_locking_lock_mutex(uint64_t mutex, bool trylock, uint64_t* error) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    pid_t holder = scheduler_current_process;

    if(!data) {
        *error = EINVAL;
        return;
    }
    else if(data->state && data->holder != holder) {
        if(trylock) {
            *error = EBUSY;
            return;
        }
        else {
            union wait_data wd;
            wd.mutex = mutex;
            scheduler_wait_for(-1, wait_reason_mutex, wd);
        }
    }
    else {
        if(!mutex_lock(mutex, holder)) {
            logw("mutex", "Unexpected mutex_lock error - we checked the same conditions. Mutex %u, new holder %d, current state %s, held by %d",
                    mutex, holder, data->state ? "locked" : "unlocked", data->holder);

            if(trylock) {
                *error = EBUSY;
                return;
            }
            else {
                union wait_data wd;
                wd.mutex = mutex;
                scheduler_wait_for(-1, wait_reason_mutex, wd);
            }
        }
    }

    *error = 0;
}

void sc_handle_locking_unlock_mutex(uint64_t mutex, uint64_t* error) {
    struct mutex_data* data = tpa_get(mutexes, mutex);

    pid_t holder = scheduler_current_process;

    if(!data) {
        *error = EINVAL;
        return;
    }
    else if(!data->state) {
        *error = 0;
        return;
    }
    else if(data->holder != holder) {
        *error = EPERM;
        return;
    }

    *error = 0;
    mutex_unlock(mutex, holder);
}
