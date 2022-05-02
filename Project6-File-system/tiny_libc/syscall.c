#include <sys/syscall.h>
// #include <sys/shm.h>
#include <stdint.h>
#include <time.h>
/*睡眠*/
void sys_sleep(uint32_t time)
{

    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

int sys_get_char()
{
    /*读取键盘输入*/
    int ch;
    /*必须获取得到一个输入才会继续并返回*/
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

void sys_write(char *buff)
{
    // TODO:
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

//初始化锁并得到id
int sys_lock_init(int *mutex_id)
{
    invoke_syscall(SYSCALL_LOCK_INIT, mutex_id, IGNORE, IGNORE, IGNORE);
}

//具体操作，上锁或者解锁
void sys_lock_op(int mutex_id, int op)
{
    invoke_syscall(SYSCALL_LOCK_OP, mutex_id, op, IGNORE, IGNORE);
}

void sys_priori(int priori, int pcb_id)
{
    invoke_syscall(SYSCALL_PRIORI, priori, pcb_id, IGNORE, IGNORE);
}

int sys_get_id()
{
    return invoke_syscall(SYSCALL_GET_ID, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_get_priorit()
{
    return invoke_syscall(SYSCALL_GET_PRIORIT, IGNORE, IGNORE, IGNORE, IGNORE);
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

/* p4 */
pid_t sys_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, mode);
}
/* p4 */
void sys_process_show(void)
{
}

int sys_kill(char *name)
{
    return invoke_syscall(SYSCALL_KILL, name, IGNORE, IGNORE, IGNORE);
}
// /* p4 */
// void sys_ls()
// {
//     invoke_syscall(SYSCALL_LS, IGNORE, IGNORE, IGNORE, IGNORE);
// }

void sys_ps()
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_taskset(int mask, char *name, int mode)
{
    /*mode 为0等同于创建，为1是在中途更改*/
    invoke_syscall(SYSCALL_TASKSET, mask, name, mode, IGNORE);
}

/*信号量的结构*/
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

/*信箱*/
void sys_mailbox_open(int *mailbox_id, char *name)
{
    invoke_syscall(SYSCALL_MBOX_OPEN, mailbox_id, name, IGNORE, IGNORE);
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
    return invoke_syscall(SYSCALL_MBOX_RECV, mailbox_id, msg, msg_length, IGNORE);
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
    return invoke_syscall(SYSCALL_THREAD_CREAT, entry_point, arg, IGNORE, IGNORE);
}

int thread_join(int thread)
{
    return invoke_syscall(SYSCALL_THREAD_JION, thread, IGNORE, IGNORE, IGNORE);
}

int sys_fork()
{
    return invoke_syscall(SYSCALL_FORK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void *shmpageget(int key)
{
    return (void *)invoke_syscall(SYSCALL_SHAM_PAGE_GET, key, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr)
{
    invoke_syscall(SYSCALL_SHAM_PAGE_DT, addr, IGNORE, IGNORE, IGNORE);
}

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t *frLength)
{
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}
void sys_net_send(uintptr_t addr, size_t length)
{
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}
void sys_net_irq_mode(int mode)
{
    invoke_syscall(SYSCALL_NET_IRQ, mode, IGNORE, IGNORE, IGNORE);
}

/* ------------------P6---------------- */
void sys_mkfs(){
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE, IGNORE);
}
void sys_statfs(){
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}
//directory operation
void sys_cd(char *dirname){
    invoke_syscall(SYSCALL_CD, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}
void sys_mkdir(char *dirname){
    invoke_syscall(SYSCALL_MKDIR, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}
void sys_rmdir(char *dirname){
    invoke_syscall(SYSCALL_RMDIR, (uintptr_t)dirname,IGNORE, IGNORE, IGNORE);
}
void sys_ls(int mode, char * path){
    invoke_syscall(SYSCALL_LS, mode, path, IGNORE, IGNORE);
}
//file operation
void sys_touch(char *filename){
    invoke_syscall(SYSCALL_TOUCH, (uintptr_t)filename, IGNORE, IGNORE, IGNORE);
}
void sys_cat(char *filename){
    invoke_syscall(SYSCALL_CAT, (uintptr_t)filename, IGNORE, IGNORE, IGNORE);
}
int sys_fopen(char *name, int access){
    return invoke_syscall(SYSCALL_FILE_OPEN, (uintptr_t)name, access, IGNORE, IGNORE);
}
int sys_fread(int fd, char *buff, int size){
    return invoke_syscall(SYSCALL_FILE_READ, fd, (uintptr_t)buff, size, IGNORE);
}
int sys_fwrite(int fd, char *buff, int size){
    return invoke_syscall(SYSCALL_FILE_WRITE, fd, (uintptr_t)buff, size, IGNORE);
}
void sys_close(int fd){
    invoke_syscall(SYSCALL_FILE_CLOSE, fd, IGNORE, IGNORE, IGNORE);
}
void sys_ln(char *source, char *link_name){
    invoke_syscall(SYSCALL_LN, (uintptr_t)source, (uintptr_t)link_name, IGNORE, IGNORE);
}
void sys_delete(void)
{
    invoke_syscall(SYSCALL_DELETE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_lseek(int fd, int offset, int whence){
    return invoke_syscall(SYSCALL_LSEEK, fd, offset, whence, IGNORE);
}

int sys_rm(char * path){
    return invoke_syscall(SYSCALL_RM, path, IGNORE, IGNORE, IGNORE);
}