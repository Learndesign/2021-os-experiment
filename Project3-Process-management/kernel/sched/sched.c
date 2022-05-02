#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

user_sem_t user_sem_array[NUM_MAX_USER];
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_MSTER + 0x0e70;
pcb_t pid0_pcb_master = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .preempt_count = 0,
    .cursor_x = 0,
    .cursor_y = 0,
    .status = TASK_READY}; //master核
const ptr_t pid0_stack_s = INIT_KERNEL_STACK_SLAVE + 0x0e70;
pcb_t pid0_pcb_slave = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .preempt_count = 0,
    .cursor_x = 0,
    .cursor_y = 0,
    .status = TASK_READY}; //slave核

LIST_HEAD(ready_queue);
LIST_HEAD(wait_queue);
/* current running task PCB */
pcb_t *volatile current_running;
pcb_t *volatile current_running_master;
pcb_t *volatile current_running_slave;

/* global process id */
pid_t process_id = 1;

list_node_t *get_process_for_core(int cpu_id)
{
    if (cpu_id == 0)
    {
        list_node_t *find;
        list_node_t *index;
        index = get_head_node(&ready_queue);
        while (index != &ready_queue)
        {
            pcb_t *pre = (pcb_t *)((reg_t *)index - 3);
            if (pre->mask == 1 || pre->mask == 3)
                return index;
            index = index->next;
        }
    }
    else if (cpu_id != 0)
    {

        list_node_t *find;
        list_node_t *index;
        index = get_head_node(&ready_queue);
        while (index != &ready_queue)
        {
            pcb_t *pre = (pcb_t *)((reg_t *)index - 3);
            if (pre->mask == 2 || pre->mask == 3)
                return index;
            index = index->next;
        }
    }
    return &ready_queue;
}

void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    int cpu_id = get_current_cpu_id();
    current_running = cpu_id == 0 ? current_running_master : current_running_slave;

    if (current_running->status == TASK_RUNNING && current_running->pid != 0)
    {
        current_running->status = TASK_READY;
        list_add(&current_running->list, &ready_queue);
    }

    pcb_t *prev_running = current_running;
    // get new current_running  from ready_queue
    if (!list_empty(&ready_queue))
    {
        list_node_t *current_list;
        if ((current_list = get_process_for_core(cpu_id)) != &ready_queue)
        {
            list_del(current_list);
            current_running = (pcb_t *)((reg_t *)current_list - 3);
        }
        else
        {
            current_running = cpu_id == 0 ? &pid0_pcb_master : &pid0_pcb_slave;
        }
        if (cpu_id == 0)
            current_running_master = current_running; //更换
        else
            current_running_slave = current_running; //更换
    }
    else
    {
        if (current_running->status != TASK_RUNNING)
        {
            current_running = cpu_id == 0 ? &pid0_pcb_master : &pid0_pcb_slave;
            if (cpu_id == 0)
                current_running_master = current_running; //更换
            else
                current_running_slave = current_running; //更换
        }
    }

    current_running->status = TASK_RUNNING;
    // restore the current_runnint's cursor_x and cursor_y
    // vt100_move_cursor(current_running->cursor_x,
    //                   current_running->cursor_y);
    // screen_cursor_x = current_running->cursor_x;
    // screen_cursor_y = current_running->cursor_y;
    //printk("> [CHANGE] changing\n\r");
    //  TODO: switch_to current_running
    switch_to(prev_running, current_running);
    //printk("> [CHANGE] changed\n\r");
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    current_running->status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout
    int current_time = get_timer();
    uint32_t out_time = current_time + sleep_time; //得到出来的时间
    current_running->timer.timeout = out_time;
    list_add(&current_running->list, &sleep_queue);
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    current_running->status = TASK_BLOCKED;
    list_add(pcb_node, queue);
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    pcb_t *volatile pcb = (pcb_t *)((reg_t *)pcb_node - 3);
    pcb->status = TASK_READY;
    list_del(pcb_node);
    list_add(pcb_node, &ready_queue);
}

int do_fork()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int id = process_id;
    if (id >= NUM_MAX_TASK)
    {
        printk("[ERROR] 进程数量已达上限!\n");
        return -1;
    }
    pcb[id].kernel_sp = allocPage(1) - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb[id].user_sp = allocPage(1);
    int offset = pcb[id].kernel_sp - current_running->kernel_sp;
    pcb[id].user_sp = current_running->user_sp + offset;
    pcb[id].pid = process_id++;
    pcb[id].cursor_x = current_running->cursor_x;
    pcb[id].cursor_y = current_running->cursor_y;
    pcb[id].preempt_count = current_running->preempt_count;
    pcb[id].save_context = (regs_context_t *)(pcb[id].kernel_sp + sizeof(switchto_context_t));
    pcb[id].switch_context = (switchto_context_t *)(pcb[id].kernel_sp);
    pcb[id].type = current_running->type;
    pcb[id].status = TASK_BLOCKED;
    pcb[id].list.priority = 0;
    pcb[id].prepriority = 0;
    int kernel_stack = current_running->kernel_sp;
    char *new_pcb = (char *)(pcb[id].kernel_sp & 0xfffff000);
    char *old_pcb = (char *)(current_running->kernel_sp & 0xfffff000);
    /*从kernel栈基地址开始复制分配的全部俩页的内容*/
    kmemcpy(new_pcb, old_pcb, 0x2000);
    pcb[id].save_context->regs[2] = current_running->save_context->regs[2] + offset;
    pcb[id].save_context->regs[4] = (reg_t)&pcb[id];
    pcb[id].save_context->regs[8] = current_running->save_context->regs[8] + offset;
    pcb[id].switch_context->regs[1] = pcb[id].kernel_sp;
    pcb[id].switch_context->regs[0] = (reg_t)&ret_from_exception;
    /*子进程返回0，父进程返回子进程id*/
    if (current_running->pid == (process_id - 1))
        return 0;
    else
        return id;
}

int do_get_id()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    return current_running->pid;
}

int do_get_priorit()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    return current_running->list.priority;
}

void do_priori(int priori, int pcb_id)
{
    if (pcb_id)
    {
        pcb[pcb_id].list.priority = priori;
        pcb[pcb_id].prepriority = priori;
        pcb[pcb_id].status = TASK_READY;
        list_add(&pcb[pcb_id].list, &ready_queue);
    }
}

void do_process_show()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    prints("[PROCESS TABLE]\n");
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid)
        {
            prints("[%d] ", i);
            switch (pcb[i].status)
            {
            case TASK_RUNNING:
                if (current_running == &pcb[i] && get_current_cpu_id() == 0)
                    prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "RUNNING (core 0)", pcb[i].mask);
                if (current_running == &pcb[i] && get_current_cpu_id() == 1)
                    prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "RUNNING (core 1)", pcb[i].mask);
                if (current_running != &pcb[i] && get_current_cpu_id() == 0)
                    prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "RUNNING (core 1)", pcb[i].mask);
                if (current_running != &pcb[i] && get_current_cpu_id() == 1)
                    prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "RUNNING (core 0)", pcb[i].mask);
                break;
            case TASK_BLOCKED:
                prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "BLOCKED", pcb[i].mask);
                break;
            case TASK_EXITED:
                prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "EXITED", pcb[i].mask);
                break;
            case TASK_READY:
                prints("PID %d: STATUS:%s MASK = %d\n", pcb[i].pid, "READY", pcb[i].mask);
                break;
            }
        }
    }
}

void check_wait_queue(pid_t pid)
{
    if (pid == 0 || pid == 1)
        return;
    else
        do_unblock(&pcb[pid - 1].list);
}

int do_kill(pid_t pid)
{
    /*不能kill shell进程本身*/
    if (pid == 1)
    {
        return 2;
    }
    /*没有分配的pcb*/
    if (pcb[pid - 1].pid == 0)
        return 0;

    /*释放掉拥有的锁 */
    for (int i = 0; i < pcb[pid - 1].lock_num; i++)
        do_mutex_lock_release(pcb[pid - 1].locks[i]);
    pcb[pid - 1].lock_num = 0;

    /*释放掉在等待的父进程*/
    if (pcb[pid - 1].mode == ENTER_ZOMBIE_ON_EXIT)
        check_wait_queue(pcb[pid - 1].parent);

    /*父进程死亡，将子进程托管给爷爷进程，不存在爷爷进程则托管给shell进程*/
    int child = pcb[pid - 1].child;
    if (2 <= child <= 16)
    {
        if (pcb[pid - 1].mode == ENTER_ZOMBIE_ON_EXIT)
        {
            pcb[child - 1].parent = pcb[pid - 1].parent;
        }
        pcb[child - 1].parent = 1;
    }

    /*抹去pcb*/
    pcb[pid - 1]
        .status = TASK_EXITED;
    pcb[pid - 1].pid = 0;

    //释放掉队列
    list_del(&pcb[pid - 1].list);

    return 1;
}

void do_exit()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    /*释放掉拥有的锁 */
    for (int i = 0; i < current_running->lock_num; i++)
        do_mutex_lock_release(current_running->locks[i]);
    current_running->lock_num = 0;

    /*释放掉在等待的父进程*/
    if (current_running->mode == ENTER_ZOMBIE_ON_EXIT)
    {
        check_wait_queue(current_running->parent);
    }

    current_running->status = TASK_EXITED;
    current_running->pid = 0;

    do_scheduler();
}

int do_waitpid(pid_t pid)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    if (pcb[pid - 1].pid != 0)
    {
        if (pcb[pid - 1].status != TASK_EXITED)
        {
            /*子进程执行完毕父进程才能进行*/
            pcb[pid - 1].parent = ((pcb_t *)(current_running))->pid;
            ((pcb_t *)(current_running))->child = pid;
            do_block(&current_running->list, &wait_queue);
        }
    }
    return pid;
}

//task2

int do_barrier_wait(int id, int count)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    user_sem_t *barrier = &user_sem_array[id];
    barrier->current_num++;
    if (count == barrier->current_num)
    {
        while (!list_empty(&barrier->block_queue))
        {
            do_unblock(barrier->block_queue.next);
        }

        barrier->current_num = 0;
    }
    else
    {
        current_running->status = TASK_BLOCKED;
        list_add(&current_running->list, &barrier->block_queue);
        do_scheduler();
    }
    return 1;
}

void init_user_sem_array()
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
        user_sem_array[i].status = B_UNSED;
        user_sem_array[i].id = i + 1;
        init_list_head(&user_sem_array[i].block_queue);
    }
}

int user_barrier_create()
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
        if (user_sem_array[i].status != B_USED)
        {
            user_sem_array[i].status = B_USED;
            return i;
        }
    }
}

int user_sem_create(int value)
{
    for (int i = 0; i < NUM_MAX_USER; i++)
    {
        if (user_sem_array[i].status != B_USED)
        {
            user_sem_array[i].current_num = value;
            user_sem_array[i].status = B_USED;
            return i;
        }
    }
}

int user_sem_destory(int i)
{
    if (user_sem_array[i].status == B_USED)
    {
        user_sem_array[i].current_num = 0;
        user_sem_array[i].status = B_UNSED;
        init_list_head(&user_sem_array[i].block_queue);
    }
}

void user_semaphore_signal(int id)
{
    user_sem_t *s = &user_sem_array[id];
    s->current_num++;
    if (s->current_num <= 0)
    {
        if (!list_empty(&s->block_queue))
        {
            do_unblock(s->block_queue.next);
        }
    }
}

void user_semaphore_wait(int id)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    user_sem_t *s = &user_sem_array[id];
    s->current_num--;
    if (s->current_num < 0)
    {
        do_block(&current_running->list, &s->block_queue);
    }
}
