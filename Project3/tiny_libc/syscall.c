#include <sys/syscall.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}
int sys_read()
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_delete()
{
    // TODO:
    invoke_syscall(SYSCALL_DELETE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    // invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    //   or
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
    // ???
}

int sys_lock_init()
{
    return invoke_syscall(SYSCALL_LOCKCREATE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_lock_jion(int lock_id, int op)
{
    invoke_syscall(SYSCALL_LOCKJION, lock_id, op, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed)
{
    return invoke_syscall(SYSCALL_GET_WALL_TIME, time_elapsed, IGNORE, IGNORE, IGNORE);
};

int sys_getpid()
{
    return invoke_syscall(SYSCALL_GET_ID, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_get_priorit()
{
    return invoke_syscall(SYSCALL_GET_PRIORIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_priorit(int priorit, int pcb_id)
{
    invoke_syscall(SYSCALL_PRIORIT, priorit, pcb_id, IGNORE, IGNORE);
}

void sys_clear()
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

//P3task1
void sys_ps()
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_exit()
{
    return invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode, IGNORE);
}

//P3task2
void sys_barrier_wait(int id, int count)
{
    return invoke_syscall(SYSCALL_BARRIER_WAIT, id, count, IGNORE, IGNORE);
}

int sys_barrier_init()
{
    return invoke_syscall(SYSCALL_BARRIER_INIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_sem_init(int val)
{
    return invoke_syscall(SYSCALL_SEM_INIT, val, IGNORE, IGNORE, IGNORE);
}

void sys_sem_destory(int id)
{
    return invoke_syscall(SYSCALL_SEM_DESTORY, id, IGNORE, IGNORE, IGNORE);
}

void sys_sem_up(int id)
{
    return invoke_syscall(SYSCALL_SEM_UP, id, IGNORE, IGNORE, IGNORE);
}

void sys_sem_down(int id)
{
    return invoke_syscall(SYSCALL_SEM_DOWN, id, IGNORE, IGNORE, IGNORE);
}

//P3mailbox
int sys_mailbox_open(char *name)
{
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE, IGNORE);
}

void sys_mailbox_close(int mailbox_id)
{
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox_id, IGNORE, IGNORE, IGNORE);
}

int sys_mailbox_send(int mailbox_id, void *msg, int msg_length)
{
    return invoke_syscall(SYSCALL_MBOX_SEND, mailbox_id, msg, msg_length, IGNORE);
}

int sys_mailbox_recv(int mailbox_id, void *msg, int msg_length)
{
    return invoke_syscall(SYACALL_MBOX_RECV, mailbox_id, msg, msg_length, IGNORE);
}

int sys_mailbox_send_recv(int *mailbox_id, void *send_msg, void *recv_msg, int *length)
{
    return invoke_syscall(SYSCALL_MBOX_SEND_RECV, mailbox_id, send_msg, recv_msg, length);
}

int sys_taskset(task_info_t *task, int mask, int pid, int mode)
{
    invoke_syscall(SYSCALL_TASKSET, task, mask, pid, mode);
}