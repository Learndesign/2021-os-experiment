#include <sys/syscall.h>
#include <stdint.h>
#include <time.h>

void sys_sleep(uint32_t time)
{

    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

int sys_get_char()
{
    int ch;
    while (1)
    {
        ch = invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE, IGNORE);
        if (ch != -1)
            return ch;
    }
    return ch;
}

void sys_put_char(char ch)
{
    invoke_syscall(SYSCALL_PUT_CHAR, ch, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_delete(void)
{
    invoke_syscall(SYSCALL_DELETE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
}

int sys_lock_init(int *mutex_id)
{
    invoke_syscall(SYSCALL_LOCK_INIT, mutex_id, IGNORE, IGNORE, IGNORE);
}

void sys_lock_op(int mutex_id, int op)
{
    invoke_syscall(SYSCALL_LOCK_OP, mutex_id, op, IGNORE, IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, mode);
}

int sys_kill(char *name)
{
    return invoke_syscall(SYSCALL_KILL, name, IGNORE, IGNORE, IGNORE);
}

void sys_ls()
{
    invoke_syscall(SYSCALL_LS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_ps()
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_semphore_init(int *id, int *value, int val)
{
    invoke_syscall(SYSCALL_SEM_INIT, id, value, val, IGNORE);
}

void sys_semphore_up(int mutex_id, int *semphore)
{
    invoke_syscall(SYSCALL_SEM_UP, mutex_id, semphore, IGNORE, IGNORE);
}

void sys_semphore_down(int mutex_id, int *semphore)
{
    invoke_syscall(SYSCALL_SEM_DOWN, mutex_id, semphore, IGNORE, IGNORE);
}

void sys_semphore_destory(int *mutex_id, int *semphore)
{
    invoke_syscall(SYSCALL_SEM_DESTORY, mutex_id, semphore, IGNORE, IGNORE);
}

void sys_barrier_init(int *barrier_id, int *wt_now, int *wt_num, unsigned int count)
{
    invoke_syscall(SYSCALL_BARRIER_INIT, barrier_id, wt_now, wt_num, count);
}

void sys_barrier_wait(int barrer_id, int *wt_now, int wt_num)
{
    invoke_syscall(SYSCALL_BARRIER_WAIT, barrer_id, wt_now, wt_num, IGNORE);
}

void sys_barrier_destory(int *barrer_id, int *wt_now, int *wt_num)
{
    invoke_syscall(SYSCALL_BARRIER_DESTORY, barrer_id, wt_now, wt_num, IGNORE);
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

/* P4 */
int binsemget(int key)
{
    int mutex_id;
    return invoke_syscall(SYSCALL_LOCK_INIT, &mutex_id, key, IGNORE, IGNORE);
}

/* P4 */
int binsemop(int binsem_id, int op)
{
    invoke_syscall(SYSCALL_LOCK_OP, binsem_id, op, IGNORE, IGNORE);
}

int sys_create_thread(uintptr_t entry_point, void *arg)
{
    return invoke_syscall(SYSCALL_THREAD_CREATE, entry_point, arg, IGNORE, IGNORE);
}

int thread_join(int thread)
{
    return invoke_syscall(SYSCALL_THREAD_JION, thread, IGNORE, IGNORE, IGNORE);
}