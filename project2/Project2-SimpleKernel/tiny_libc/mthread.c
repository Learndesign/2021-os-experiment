#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>

int mthread_mutex_init(void *handle)
{
    /* TODO: */
    do_mutex_lock_init(handle);
    return 0;
}
int mthread_mutex_lock(void *handle)
{
    /* TODO: */
    do_mutex_lock_acquire(handle);
    return 0;
}
int mthread_mutex_unlock(void *handle)
{
    /* TODO: */
    do_mutex_lock_release(handle);
    return 0;
}