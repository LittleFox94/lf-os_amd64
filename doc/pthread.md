# pthread in LF OS

## Functions and their categories (roughly)

### Mutexes
  - attrs
    + `int pthread_mutexattr_init (pthread_mutexattr_t *__attr);`
    + `int pthread_mutexattr_destroy (pthread_mutexattr_t *__attr);`
    + `int pthread_mutexattr_getpshared (const pthread_mutexattr_t *__attr, int *__pshared);`
    + `int pthread_mutexattr_setpshared (pthread_mutexattr_t *__attr, int __pshared);`

  - `int pthread_mutex_init (pthread_mutex_t *__mutex, const pthread_mutexattr_t *__attr);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓ (well, attributes are ignored)

  - `int pthread_mutex_destroy (pthread_mutex_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_mutex_lock (pthread_mutex_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_mutex_trylock (pthread_mutex_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_mutex_unlock (pthread_mutex_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

### Condvars + attrs
  - attr
    + `int pthread_condattr_init (pthread_condattr_t *__attr);`
    + `int pthread_condattr_destroy (pthread_condattr_t *__attr);`
    + `int pthread_condattr_getclock (const pthread_condattr_t *__restrict __attr, clockid_t *__restrict __clock_id);`
    + `int pthread_condattr_setclock (pthread_condattr_t *__attr, clockid_t __clock_id);`
    + `int pthread_condattr_getpshared (const pthread_condattr_t *__attr, int *__pshared);`
    + `int pthread_condattr_setpshared (pthread_condattr_t *__attr, int __pshared);`

  - `int pthread_cond_init (pthread_cond_t *__cond, const pthread_condattr_t *__attr);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓ (well, attributes are ignored)

  - `int pthread_cond_destroy (pthread_cond_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_cond_signal (pthread_cond_t *__cond);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_cond_broadcast (pthread_cond_t *__cond);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_cond_wait (pthread_cond_t *__cond, pthread_mutex_t *__mutex);`
    + syscall definition: ✓
    + syscall implementation: ✓
    + userspace implementation: ✓

  - `int pthread_cond_timedwait (pthread_cond_t *__cond, pthread_mutex_t *__mutex, const struct timespec *__abstime);`
    + **returns ENOTSUP for now** (no timer yet)


### Threads + attrs
  - attr
    + `int pthread_attr_setschedparam (pthread_attr_t *__attr, const struct sched_param *__param);`
    + `int pthread_attr_getschedparam (const pthread_attr_t *__attr, struct sched_param *__param);`
    + `int pthread_attr_init (pthread_attr_t *__attr);`
    + `int pthread_attr_destroy (pthread_attr_t *__attr);`
    + `int pthread_attr_setstack (pthread_attr_t *attr, void *__stackaddr, size_t __stacksize);`
    + `int pthread_attr_getstack (const pthread_attr_t *attr, void **__stackaddr, size_t *__stacksize);`
    + `int pthread_attr_getstacksize (const pthread_attr_t *__attr, size_t *__stacksize);`
    + `int pthread_attr_setstacksize (pthread_attr_t *__attr, size_t __stacksize);`
    + `int pthread_attr_getstackaddr (const pthread_attr_t *__attr, void **__stackaddr);`
    + `int pthread_attr_setstackaddr (pthread_attr_t *__attr, void *__stackaddr);`
    + `int pthread_attr_getdetachstate (const pthread_attr_t *__attr, int *__detachstate);`
    + `int pthread_attr_setdetachstate (pthread_attr_t *__attr, int __detachstate);`
    + `int pthread_attr_getguardsize (const pthread_attr_t *__attr, size_t *__guardsize);`
    + `int pthread_attr_setguardsize (pthread_attr_t *__attr, size_t __guardsize);`
  - `int pthread_create (pthread_t *__pthread, const pthread_attr_t *__attr, void *(*__start_routine)(void *), void *__arg);`
  - `int pthread_join (pthread_t __pthread, void **__value_ptr);`
  - `int pthread_detach (pthread_t __pthread);`
  - `void pthread_exit (void *__value_ptr) __dead2;`
  - `int pthread_cancel (pthread_t __pthread);`
  - `pthread_t pthread_self (void);`

### utils (cleanup, atfork, ..)
  - `int pthread_atfork (void (*prepare)(void), void (*parent)(void), void (*child)(void));`
  - `int pthread_equal (pthread_t __t1, pthread_t __t2);`
  - `int pthread_getcpuclockid (pthread_t thread, clockid_t *clock_id);`
  - `int pthread_setconcurrency (int new_level);`
  - `int pthread_getconcurrency (void);`
  - `int pthread_once (pthread_once_t *__once_control, void (*__init_routine)(void));`
  - `int pthread_key_create (pthread_key_t *__key, void (*__destructor)(void *));`
  - `int pthread_setspecific (pthread_key_t __key, const void *__value);`
  - `void * pthread_getspecific (pthread_key_t __key);`
  - `int pthread_key_delete (pthread_key_t __key);`
  - `int pthread_setcancelstate (int __state, int *__oldstate);`
  - `int pthread_setcanceltype (int __type, int *__oldtype);`
  - `void pthread_testcancel (void);`
  - `void _pthread_cleanup_push (struct _pthread_cleanup_context *_context, void (*_routine)(void *), void *_arg);`
  - `void _pthread_cleanup_pop (struct _pthread_cleanup_context *_context, int _execute);`
