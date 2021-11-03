#include <sys/syscall.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}
char sys_read()
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    // invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    //   or
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    // ???
}

int sys_lock_init()
{
    return invoke_syscall(SYSCALL_LOCKCREATE, IGNORE, IGNORE, IGNORE);
}

void sys_lock_jion(int lock_id, int op)
{
    invoke_syscall(SYSCALL_LOCKJION, lock_id, op, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed)
{
    return invoke_syscall(SYSCALL_GET_WALL_TIME, time_elapsed, IGNORE, IGNORE);
};

int sys_get_id()
{
    return invoke_syscall(SYSCALL_GET_ID, IGNORE, IGNORE, IGNORE);
}

int sys_get_priorit()
{
    return invoke_syscall(SYSCALL_GET_PRIORIT, IGNORE, IGNORE, IGNORE);
}

void sys_priori(int priorit, int pcb_id)
{
    invoke_syscall(SYSCALL_PRIORIT, priorit, pcb_id, IGNORE);
}