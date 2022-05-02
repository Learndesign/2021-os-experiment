#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
user_lock_t user_lock_array[NUM_MAX_USER];
void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status = UNLOCKED;
    init_list_head(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    if (lock->lock.status == LOCKED)
    {
        current_running->status = TASK_BLOCKED;
        do_block(&current_running->list, &lock->block_queue);
    }
    else
        lock->lock.status = LOCKED;
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    if (!list_empty(&lock->block_queue))
    {
        do_unblock(lock->block_queue.next);
        lock->lock.status = LOCKED;
    }
    else
        lock->lock.status = UNLOCKED;
}

void init_user_lock_array()
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
        user_lock_array[i].lock.status = UNSED;
        user_lock_array[i].id = i + 1;
    }
}

int lock_create()
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
        if (user_lock_array[i].kernel_lock.lock.status != LOCKED && user_lock_array[i].lock.status != USED)
        {
            user_lock_array[i].lock.status = USED;
            do_mutex_lock_init(&user_lock_array[i].kernel_lock);
            return i;
        }
    }
}

void lock_jion(int lock_id, int op)
{
    if (op == USER_OP_LOCK)
        do_mutex_lock_acquire(&user_lock_array[lock_id].kernel_lock);
    else
        do_mutex_lock_release(&user_lock_array[lock_id].kernel_lock);
}
