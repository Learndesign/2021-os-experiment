#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status = UNLOCKED;
    init_list_head(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    if(lock->lock.status == LOCKED){
        current_running->status = TASK_BLOCKED;
        do_block(&current_running->list, &lock->block_queue);
    }
    else
        lock->lock.status = LOCKED;
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    if(!list_empty(&lock->block_queue)){
        do_unblock(lock->block_queue.next);
        lock->lock.status = LOCKED;
    }
    else
        lock->lock.status = UNLOCKED;
}
