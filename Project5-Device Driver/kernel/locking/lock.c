#include <os/lock.h>
#include <os/sched.h>
#include <os/mm.h>
#include <atomic.h>
#include <screen.h>
#include <os/string.h>

void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    init_list(&lock->block_queue);
    lock->lock.status = UNLOCKED;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    if (lock->lock.status == LOCKED)
    {
        do_block(&current_running->list, &lock->block_queue);
    }
    else
    {
        lock->lock.status = LOCKED;
        kernel_lock_t *use_lock = list_entry(lock, kernel_lock_t, kernel_lock);
        for (int i = 0; i < MAX_LOCK_NUM; i++)
        {
            if (current_running->handle_mutex[i] == -1)
            {
                current_running->handle_mutex[i] = use_lock->id;
                break;
            }
        }
        use_lock->status = USED;
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    int i;
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    kernel_lock_t *use_lock = list_entry(lock, kernel_lock_t, kernel_lock);
    for (i = 0; i < MAX_LOCK_NUM; i++)
    {
        if (use_lock->id == current_running->handle_mutex[i])
        {
            current_running->handle_mutex[i] = -1; //标记该进程没有持有这把锁。
            break;
        }
    }
    if (i >= MAX_LOCK_NUM)
    {
        prints("> [ERROR] : the process() dosen't handle this mutex!\n" /*, current_running->pid*/);
        return;
    }
    if (!list_empty(&lock->block_queue))
    {
        pcb_t *unblock = list_entry(lock->block_queue.prev, pcb_t, list);
        do_unblock(lock->block_queue.prev); //释放一个锁进程
        for (int i = 0; i < MAX_LOCK_NUM; i++)
        {
            if (unblock->handle_mutex[i] == -1)
            {
                unblock->handle_mutex[i] = use_lock->id; //该进程持有锁
                break;
            }
        }
    }
    else
    {
        lock->lock.status = UNLOCKED;
        //更改用户锁表示没有使用过
        kernel_lock_t *pre = list_entry(lock, kernel_lock_t, kernel_lock);
        pre->status = UNSED;
    }
}

/*所有的锁都进行初始化*/
void init_source_array()
{
    for (int i = 0; i < MAX_LOCK_NUM; i++)
    {
        /*初始化锁*/
        user_lock_array[i].status = UNSED;
        user_lock_array[i].id = i;
        user_lock_array[i].key = -1;
        /*初始化信号量*/
        user_semphore_array[i].status = UNSED;
        /*初始化屏障*/
        user_barrier_array[i].status = UNSED;
        /*初始化信箱*/
        init_user_mailbox(i);
    }
}

void init_user_mailbox(int i)
{
    user_mailbox_array[i].visited = -1;
    user_mailbox_array[i].name = 0;
    user_mailbox_array[i].head = 0;
    user_mailbox_array[i].tail = 0;
    user_mailbox_array[i].msg_num = 0;
    user_mailbox_array[i].status = MBOX_CLOSE;
    init_list(&user_mailbox_array[i].empty);
    init_list(&user_mailbox_array[i].full);
}

int do_mutex_init(int *mutex_id, int key)
{
    int i = 0;
    for (i = 0; i < MAX_LOCK_NUM; i++)
    {
        if (user_lock_array[i].key == key)
            return i;
    }
    for (i = 0; i < MAX_LOCK_NUM; i++)
    {
        if (user_lock_array[i].kernel_lock.lock.status != LOCKED && user_lock_array[i].status != USED)
        {
            user_lock_array[i].status = USED;
            user_lock_array[i].key = key;
            do_mutex_lock_init(&user_lock_array[i].kernel_lock);
            *mutex_id = i;
            return i;
        }
    }
    if (i >= MAX_LOCK_NUM)
    {
        // screen_move_cursor(1,4);
        prints("> [ERROR] : not find one free lock\n");
        return -1;
        while (1)
            ;
    }
}

void do_mutex_op(int mutex_id, int op)
{
    if (op == 0)
        //上锁
        do_mutex_lock_acquire(&user_lock_array[mutex_id].kernel_lock);
    else
        //解锁
        do_mutex_lock_release(&user_lock_array[mutex_id].kernel_lock);
}

/*信号量*/
void do_semphore_init(int *id, int *value, int val)
{
    /*初始化信号量即为直接初始化锁结构就行*/
    int i;
    for (i = 0; i < MAX_WAIT_QUEUE; i++)
    {
        if (user_semphore_array[i].status == UNSED)
        {
            user_semphore_array[i].status = USED;
            user_semphore_array[i].wt_now = 0;
            user_semphore_array[i].wt_num = val;
            init_list(&user_semphore_array[i].wait_queue);
            break;
        }
    }
    if (i > MAX_WAIT_QUEUE)
    {
        prints("> [ERROR] : no more resoure!\n");
        while (1)
            ;
    }
    (*id) = i;
    (*value) = val;
}

void do_semphore_up(int mutex_id, int *semphore)
{
    if (mutex_id == -1)
    {
        // screen_move_cursor(1,4);
        prints("> [ERROR] :semphore is not exist\n");
        return;
    }
    list_node_t *wt = &user_semphore_array[mutex_id].wait_queue;
    if (!list_empty(wt))
    {
        do_unblock(wt->prev);
    }
    (*semphore)++;
}

void do_semphore_down(int mutex_id, int *semphore)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    if (mutex_id == -1)
    {
        prints("> [ERROR] :semphore is not exist.\n");
        return;
    }
    (*semphore)--;
    if ((*semphore) < 0)
    {
        do_block(&current_running->list, &user_semphore_array[mutex_id].wait_queue);
    }
}

void do_semphore_destory(int *mutex_id, int *semphore)
{
    kernel_semwait_t des_sem = user_semphore_array[*mutex_id];
    list_node_t *des_list = &user_semphore_array[*mutex_id].wait_queue;
    while (!list_empty(des_list))
    {
        do_unblock(des_list->prev);
    }
    (*mutex_id) = -1;
    (*semphore) = 0;
    des_sem.status = UNSED;
}
/*屏障*/
void do_barrier_init(int *barrier_id, int *wt_now, int *wt_num, unsigned count)
{
    int i;
    for (i = 0; i < MAX_BARRIER_NUM; i++)
    {
        if (user_barrier_array[i].status == UNSED)
        {
            init_list(&user_barrier_array[i].wait_queue);
            *barrier_id = i;
            *wt_now = 0;
            *wt_num = count;
            break;
        }
    }
    if (i > MAX_BARRIER_NUM)
    {
        // screen_move_cursor(1,4);
        prints("> [ERROR] : no more resoure!\n");
        while (1)
            ;
    }
}
void do_barrier_wait(int barrier_id, int *wt_now, int wt_num)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    if (barrier_id == -1)
    {
        // screen_move_cursor(1,4);
        prints("> [ERROR] : no this Barrier, it has been deleted!\n");
        return;
    }
    if ((++*wt_now) == wt_num)
    {
        while (!list_empty(&user_barrier_array[barrier_id].wait_queue))
        {
            do_unblock(user_barrier_array[barrier_id].wait_queue.prev);
        }
        *wt_now = 0;
    }
    else
    {
        do_block(&current_running->list, &user_barrier_array[barrier_id].wait_queue);
    }
}

void do_barrier_destory(int *barrier_id, int *wt_now, int *wt_num)
{
    user_barrier_array[*barrier_id].status = UNSED;
    *barrier_id = -1;
    *wt_now = 0;
    *wt_num = 0;
}

/*mailbox通信*/
int do_mbox_open(char *name)
{
    int mailbox_id;
    int i = 0;
    for (i = 0; i < MAX_MAILBOX_NUM; i++)
    {
        if (user_mailbox_array[i].visited != -1 && kstrcmp(user_mailbox_array[i].name, name) == 0)
        {
            user_mailbox_array[i].visited++;
            mailbox_id = i;
            return mailbox_id;
        }
    }
    for (i = 0; i < MAX_MAILBOX_NUM; i++)
    {
        if (user_mailbox_array[i].visited == -1)
        {
            user_mailbox_array[i].status = MBOX_OPEN;
            user_mailbox_array[i].visited = 1;
            int mutex_id;
            do_mutex_init(&mutex_id, 5);
            user_mailbox_array[i].mbox_lock = &user_lock_array[mutex_id].kernel_lock;

            int j = 0;
            user_mailbox_array[i].name = (char *)kmalloc(sizeof(char) * kstrlen(name));
            while (*name)
            {
                user_mailbox_array[i].name[j++] = *name;
                name++;
            }
            user_mailbox_array[i].name[j] = '\0';
            mailbox_id = i;
            return mailbox_id;
        }
    }
    prints(">[ERROR] : No more mailbox");
    return;
}
void do_mbox_close(int mailbox_id)
{
    user_mailbox_array[mailbox_id].status = MBOX_CLOSE;
    user_mailbox_array[mailbox_id].visited--;
}

int do_mbox_send(int mailbox_id, void *msg, int msg_length)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int send_block_time = 0;
    while (user_mailbox_array[mailbox_id].msg_num + msg_length > MAX_MSG_LENGTH)
    {
        send_block_time++;
        do_block(&current_running->list, &user_mailbox_array[mailbox_id].full);
    }

    user_mailbox_array[mailbox_id].msg_num += msg_length;
    for (int i = 0; i < msg_length; i++)
    {
        user_mailbox_array[mailbox_id].msg[(user_mailbox_array[mailbox_id].head++) % MAX_MSG_LENGTH] = ((char *)msg)[i];
    }
    user_mailbox_array[mailbox_id].head = (user_mailbox_array[mailbox_id].head) % MAX_MSG_LENGTH;

    while (!list_empty(&user_mailbox_array[mailbox_id].empty))
    {
        do_unblock(user_mailbox_array[mailbox_id].empty.prev);
    }
    return send_block_time;
}

int do_mbox_recv(int mailbox_id, void *msg, int msg_length)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int recv_block_time = 0;
    while (user_mailbox_array[mailbox_id].msg_num < msg_length)
    {
        recv_block_time++;
        do_block(&current_running->list, &user_mailbox_array[mailbox_id].empty);
    }

    user_mailbox_array[mailbox_id].msg_num -= msg_length;
    for (int i = 0; i < msg_length; i++)
    {
        ((char *)msg)[i] = user_mailbox_array[mailbox_id].msg[(user_mailbox_array[mailbox_id].tail++) % MAX_MSG_LENGTH];
    }
    user_mailbox_array[mailbox_id].tail = user_mailbox_array[mailbox_id].tail % MAX_MSG_LENGTH;

    while (!list_empty(&user_mailbox_array[mailbox_id].full))
    {
        do_unblock(user_mailbox_array[mailbox_id].full.prev);
    }
    return recv_block_time;
}

int do_mbox_send_recv(int *mailbox_id, void *send_msg, void *recv_msg, int *length)
{
    //返回1为send，返回0为recv
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int type = 0;
    int block_time = 0;
    int send_length = length[0];
    int recv_length = length[1];

    int send_mailbox_id = mailbox_id[0];
    int recv_mailbox_id = mailbox_id[1];

    while (user_mailbox_array[recv_mailbox_id].msg_num < recv_length && user_mailbox_array[send_mailbox_id].msg_num + send_length > MAX_MSG_LENGTH)
    {
        block_time++;
        do_block(&current_running->list, &user_mailbox_array[recv_mailbox_id].empty);
    }
    if (user_mailbox_array[recv_mailbox_id].msg_num >= recv_length)
    {
        int i;
        type = 1;
        user_mailbox_array[recv_mailbox_id].msg_num -= recv_length;
        for (i = 0; i < recv_length; i++)
        {
            ((char *)recv_msg)[i] = user_mailbox_array[recv_mailbox_id].msg[(user_mailbox_array[recv_mailbox_id].tail++) % MAX_MSG_LENGTH];
        }
        user_mailbox_array[recv_mailbox_id].tail = user_mailbox_array[recv_mailbox_id].tail % MAX_MSG_LENGTH;
    }
    if (user_mailbox_array[send_mailbox_id].msg_num + send_length <= MAX_MSG_LENGTH)
    {
        if (type == 1)
            type = 3;
        else if (type = 0)
            type = 2;
        int i;
        user_mailbox_array[send_mailbox_id].msg_num += send_length;
        for (i = 0; i < send_length; i++)
        {
            user_mailbox_array[send_mailbox_id].msg[(user_mailbox_array[send_mailbox_id].head++) % MAX_MSG_LENGTH] = ((char *)send_msg)[i];
        }
        user_mailbox_array[send_mailbox_id].head = (user_mailbox_array[send_mailbox_id].head) % MAX_MSG_LENGTH;
    }
    while (!list_empty(&user_mailbox_array[recv_mailbox_id].empty))
    {
        //唤醒所有的发送进程
        do_unblock(user_mailbox_array[recv_mailbox_id].empty.prev);
    }
    return type;
}
