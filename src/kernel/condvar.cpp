#include <condvar.h>
#include <tpa.h>
#include <vm.h>
#include <log.h>
#include <scheduler.h>
#include <unused_param.h>

struct condvar_data {
    //! Number of processes currently waiting on this condvar
    uint64_t wait_count;
};

static TPA<condvar_data>* condvars;
static uint64_t           next_condvar = 1;

void init_condvar(void) {
    condvars = TPA<condvar_data>::create(&kernel_alloc, 4080, 0);
}

void sc_handle_locking_create_condvar(uint64_t* cv, uint64_t* e) {
    if(!next_condvar) {
        *e = 12; // ENOMEM TODO: errno.h
        logw("condvar", "CondVar namespace overflow!");
        return;
    }

    struct condvar_data data = {
        .wait_count = 0,
    };

    condvars->set(next_condvar, &data);

    logd("condvar", "Created new condvar %u", next_condvar);
    *cv = next_condvar++;
    *e  = 0;
}

void sc_handle_locking_destroy_condvar(uint64_t condvar, uint64_t* e) {
    struct condvar_data* data = condvars->get(condvar);

    if(!data) {
        *e = 22; // EINVAL
        return;
    }
    else if(data->wait_count) {
        *e = 16; // EBUSY
        return;
    }

    condvars->set(condvar, 0);
    logd("condvar", "Created new condvar %u", next_condvar);
    *e  = 0;
}

void sc_handle_locking_signal_condvar(uint64_t condvar, uint64_t amount, uint64_t* e) {
    struct condvar_data* data = condvars->get(condvar);

    if(!data) {
        *e = 22; // EINVAL
        return;
    }
    else if(data->wait_count) {
        *e = 0;

        union wait_data wd;
        wd.condvar = condvar;
        scheduler_waitable_done(wait_reason_condvar, wd, amount);
        //XXX: update wait_count
    }
}

void sc_handle_locking_wait_condvar(uint64_t condvar, uint64_t timeout, uint64_t* e) {
    UNUSED_PARAM(timeout);

    struct condvar_data* data = condvars->get(condvar);

    if(!data) {
        *e = 22; // EINVAL
        return;
    }

    *e = 0;
    ++data->wait_count;

    union wait_data wd;
    wd.condvar = condvar;
    scheduler_wait_for(-1, wait_reason_condvar, wd);
}
