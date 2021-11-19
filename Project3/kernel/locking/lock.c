#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <os/string.h>
user_lock_t user_lock_array[NUM_MAX_USER];
void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status = UNLOCKED;
    init_list_head(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    /* TODO */
    if (lock->lock.status == LOCKED)
    {
        do_block(&current_running->list, &lock->block_queue);
    }
    else
    {
        lock->lock.status = LOCKED;
        /*申请锁做标记*/
        current_running->locks[current_running->lock_num++] = lock;
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    /* TODO */
    for (int i = 0; i < current_running->lock_num; i++)
    {
        if (current_running->locks[i] == lock)
        {
            int j = i + 1;
            for (; j < current_running->lock_num; j++)
            {
                current_running->locks[j - 1] = current_running->locks[j];
            }
            current_running->lock_num--;
            break;
        }
    }
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

//P3mailbox
void init_user_mailbox_array()
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
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
    init_list_head(&user_mailbox_array[i].empty);
    init_list_head(&user_mailbox_array[i].full);
}

int do_mbox_open(char *name)
{
    int mailbox_id;
    //首先根据名字寻找一个对应的mailbox
    for (int i = 0; i < MAX_MAILBOX_NUM; i++)
    {
        if (kstrcmp(user_mailbox_array[i].name, name) == 0)
        {
            user_mailbox_array[i].visited++;
            mailbox_id = i;
            return;
        }
    }

    //找不到就创建一个新的信箱
    for (int i = 0; i < MAX_MAILBOX_NUM; i++)
    {
        if (user_mailbox_array[i].visited == -1)
        {
            user_mailbox_array[i].name = (char *)kmalloc(sizeof(char) * strlen(name));
            int j = 0;
            while (*name)
            {
                user_mailbox_array[i].name[j++] = *name;
                name++;
            }
            user_mailbox_array[i].name[j] = '\0';

            user_mailbox_array[i].visited = 1;
            user_mailbox_array[i].status = MBOX_OPEN;
            //找一把锁
            int mutex_id = lock_create();
            user_mailbox_array[i].mbox_lock = &user_lock_array[mutex_id].kernel_lock;

            mailbox_id = i;
            return;
        }
    }
    //邮箱不够用
    prints(">[ERROR]: Now no mailbox can be used");
    return;
}

void do_mbox_close(int mailbox_id)
{
    /* 关闭该mailbox，如果该mailbox的引用数为0， 则释放该mailbox*/
    user_mailbox_array[mailbox_id].status = MBOX_CLOSE;
    if (user_mailbox_array[mailbox_id].visited == 0)
    {
        init_user_mailbox(mailbox_id);

        kmemset(user_mailbox_array[mailbox_id].msg, 0, MAX_MSG_LENGTH);

        user_lock_array[mailbox_id].lock.status = UNSED;
        while (!list_empty(&user_lock_array[mailbox_id].kernel_lock.block_queue))
        {
            do_mutex_lock_release(&user_lock_array[mailbox_id].kernel_lock);
        }
    }
}

int do_mbox_send(int mailbox_id, void *msg, int msg_length)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;

    int send_block_num = 0; //发送因为是满被阻塞的次数

    /*申请信箱*/
    /*有大内核锁时不需要，因为永远只有一个内核态进程*/
    // do_mutex_lock_acquire(user_mailbox_array[mailbox_id].mbox_lock);

    /*信箱放不下要发送的消息*/
    while (user_mailbox_array[mailbox_id].msg_num + msg_length > MAX_MSG_LENGTH)
    {
        //满的时候释放信箱
        // do_mutex_lock_release(user_mailbox_array[mailbox_id].mbox_lock);
        //将发送进程阻塞到full队列上
        do_block(&current_running->list, &user_mailbox_array[mailbox_id].full);
        send_block_num++;
        //重新申请信箱直到能写入
        // do_mutex_lock_acquire(user_mailbox_array[mailbox_id].mbox_lock);
    }

    /*信封放到信箱*/
    user_mailbox_array[mailbox_id].msg_num += msg_length;
    for (int i = 0; i < msg_length; i++)
    {
        user_mailbox_array[mailbox_id].msg[(user_mailbox_array[mailbox_id].head++) % MAX_MSG_LENGTH] = ((char *)msg)[i];
    }
    user_mailbox_array[mailbox_id].head = (user_mailbox_array[mailbox_id].head) % MAX_MSG_LENGTH;

    /*唤醒所有的等待这个邮箱的接收进程*/
    while (!list_empty(&user_mailbox_array[mailbox_id].empty))
    {
        do_unblock(user_mailbox_array[mailbox_id].empty.next);
    }
    // do_mutex_lock_release(user_mailbox_array[mailbox_id].mbox_lock);
    return send_block_num;
}

int do_mbox_recv(int mailbox_id, void *msg, int msg_length)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int recv_block_time = 0; //接收因为是空而被阻塞的次数

    /*申请信箱*/
    // do_mutex_lock_acquire(user_mailbox_array[mailbox_id].mbox_lock);
    while (user_mailbox_array[mailbox_id].msg_num < msg_length)
    {
        //拿不到信释放信箱
        // do_mutex_lock_release(user_mailbox_array[mailbox_id].mbox_lock);
        //将接收进程阻塞到empty队列上
        recv_block_time++;
        do_block(&current_running->list, &user_mailbox_array[mailbox_id].empty);
        //重新申请信箱直到能拿到信
        // do_mutex_lock_acquire(user_mailbox_array[mailbox_id].mbox_lock);
    }

    /*把信拿出来*/
    user_mailbox_array[mailbox_id].msg_num -= msg_length;
    for (int i = 0; i < msg_length; i++)
    {
        ((char *)msg)[i] = user_mailbox_array[mailbox_id].msg[(user_mailbox_array[mailbox_id].tail++) % MAX_MSG_LENGTH];
    }
    user_mailbox_array[mailbox_id].tail = user_mailbox_array[mailbox_id].tail % MAX_MSG_LENGTH;

    while (!list_empty(&user_mailbox_array[mailbox_id].full))
    {
        //唤醒所有的发送进程
        do_unblock(user_mailbox_array[mailbox_id].full.next);
    }
    // do_mutex_lock_release(user_mailbox_array[mailbox_id].mbox_lock);
    return recv_block_time;
}

int do_mbox_send_recv(int *mailbox_id, void *send_msg, void *recv_msg, int *length)
{
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

    /*能收则收*/
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

    /*能发则发*/
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
        do_unblock(user_mailbox_array[recv_mailbox_id].empty.next);
    }
    return type; //返回1为send，返回0为recv,3为and
}