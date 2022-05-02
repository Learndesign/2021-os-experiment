#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>
int mthread_mutex_init(mthread_mutex_t *lock)
{
    lock->lock_id = sys_lock_init();
    return 1;
}

int mthread_mutex_lock(mthread_mutex_t *lock)
{
    sys_lock_jion(lock->lock_id, USER_OP_LOCK);
    return 1;
}
int mthread_mutex_unlock(mthread_mutex_t *lock)
{
    sys_lock_jion(lock->lock_id, USER_OP_UNLOCK);
    return 1;
}

int mthread_barrier_init(mthread_barrier_t *barrier, unsigned count)
{
    barrier->id = sys_barrier_init();
    barrier->count = count;
}

int mthread_barrier_wait(mthread_barrier_t *barrier)
{
    sys_barrier_wait(barrier->id, barrier->count);
}

int mthread_barrier_destroy(mthread_barrier_t *barrier)
{
    barrier->id = 0;
    barrier->count = 0;
    sys_sem_destory(barrier->id);
}

int mthread_semaphore_init(mthread_semaphore_t *sem, int val)
{
    //可以和barrier使用同一种
    sem->id = sys_sem_init(val);
}
int mthread_semaphore_up(mthread_semaphore_t *sem)
{
    sys_sem_up(sem->id);
}
int mthread_semaphore_down(mthread_semaphore_t *sem)
{
    sys_sem_down(sem->id);
}
int mthread_semaphore_destroy(mthread_semaphore_t *sem)
{
    sem->id = 0;
    sys_sem_destory(sem->id);
}
