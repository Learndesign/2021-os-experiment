#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>

int mthread_mutex_init(void *handle)
{
    /* TODO: */
    lock_create(handle);
    return 0;
}
int mthread_mutex_lock(void *handle)
{
    /* TODO: */
    lock_jion(handle, USER_OP_LOCK);
    return 0;
}
int mthread_mutex_unlock(void *handle)
{
    /* TODO: */
    lock_jion(handle, USER_OP_UNLOCK);
    return 0;
}
