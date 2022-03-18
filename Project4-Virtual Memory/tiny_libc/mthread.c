#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>

int mthread_mutex_init(void* handle)
{
    /* TODO: */
    mthread_mutex_t * lock = (mthread_mutex_t *)handle;
    sys_lock_init(&lock->id);
    return 0;
}
int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    mthread_mutex_t * lock = (mthread_mutex_t *)handle;
    sys_lock_op(lock->id,USER_LOCK);
    return 0;
}
int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    mthread_mutex_t * lock = (mthread_mutex_t *)handle;
    sys_lock_op(lock->id,USER_NULOCK);
    return 0;
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
    mthread_barrier_t * barrier = (mthread_barrier_t *)handle;
    sys_barrier_init(&barrier->id, &barrier->wt_now, &barrier->wt_num, count);

}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    mthread_barrier_t * barrier = (mthread_barrier_t *)handle;
    sys_barrier_wait(barrier->id, &barrier->wt_now, barrier->wt_num);

}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
    mthread_barrier_t * barrier = (mthread_barrier_t *)handle;

}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
    mthread_semaphore_t * sem = (mthread_semaphore_t *) handle;
    sys_semphore_init(&sem->id,&sem->value, val);
    return 0;
}

int mthread_semaphore_up(void* handle)
{
    // TODO:
    mthread_semaphore_t * sem = (mthread_semaphore_t *) handle;
    sys_semphore_up(sem->id, &sem->value);
    return 0;
}

int mthread_semaphore_down(void* handle)
{
    // TODO:
    mthread_semaphore_t * sem = (mthread_semaphore_t *) handle;
    sys_semphore_down(sem->id, &sem->value);
    return 0;
}
int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    mthread_semaphore_t * sem = (mthread_semaphore_t *) handle;
    sys_semphore_destory(&sem->id, &sem->value);
    return 0;
}

int mthread_create(mthread_t *thread, void (*start_routine)(void*), void *arg){
    *thread =  sys_create_thread(start_routine, arg);
}

int mthread_join(mthread_t thread){
    return thread_join(thread);
}
