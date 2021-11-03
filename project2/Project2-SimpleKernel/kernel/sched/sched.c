#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0};

LIST_HEAD(ready_queue);

/* current running task PCB */
pcb_t *volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    timer_check();
    if (current_running->status != TASK_BLOCKED && current_running->pid != 0)
    {
        current_running->status = TASK_READY;
        if (current_running->list.priority > 0)
            current_running->list.priority--;
        else
            current_running->list.priority = current_running->prepriority;
        //list_add(&current_running->list, &ready_queue);
        list_add_priority(&current_running->list, &ready_queue);
    }

    pcb_t *prev_running = current_running;
    // get new current_running  from ready_queue

    if (!list_empty(&ready_queue))
    {
        list_node_t *current_list = get_head_node(&ready_queue);
        list_del(current_list);
        current_running = (pcb_t *)((reg_t *)current_list - 3);
        current_running->status = TASK_RUNNING;
    }

    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
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
    current_running->status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout
    uint32_t out_time = get_timer() + sleep_time; //得到出来的时间
    current_running->timer.timeout = out_time;
    list_add(&current_running->list, &sleep_queue);
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    list_add(pcb_node, queue);
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    list_del(pcb_node);
    list_add(pcb_node, &ready_queue);
}

int do_fork()
{
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
    return current_running->pid;
}

int do_get_priorit()
{
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