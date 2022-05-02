#include <sbi.h>
#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>

spin_lock_t kernel_lock;

void smp_init()
{
    /* TODO: */
    kernel_lock.status = UNLOCKED;
}

void wakeup_other_hart()
{
    sbi_send_ipi(NULL);
    __asm__ __volatile__(
        "csrw sip, zero");
}

void lock_kernel()
{
    while (atomic_cmpxchg(UNLOCKED, LOCKED, (ptr_t)&kernel_lock.status))
        ;
}

void unlock_kernel()
{
    kernel_lock.status = UNLOCKED;
}
