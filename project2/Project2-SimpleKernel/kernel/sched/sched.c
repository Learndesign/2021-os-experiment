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
    .preempt_count = 0
};

LIST_HEAD(ready_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;


void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
 
    if(current_running->status != TASK_BLOCKED && current_running->pid != 0){
        current_running->status=TASK_READY;
        list_add(&current_running->list,&ready_queue);
    }

    pcb_t *prev_running = current_running;
    //get new current_running  from ready_queue

    if (!list_empty(&ready_queue)){
        list_node_t *current_list = get_head_node(&ready_queue);
        current_running = (pcb_t *)((reg_t *)current_list - 3);
        current_running->status=TASK_RUNNING;
    }

    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    //printk("> [CHANGE] changing\n\r");
    // TODO: switch_to current_running
    switch_to(prev_running, current_running);
    //printk("> [CHANGE] changed\n\r");
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
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
