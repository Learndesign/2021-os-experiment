#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <os/sched.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint32_t time_base = 0;

//睡眠队列
LIST_HEAD(sleep_queue);

// time_base存储了处理器1秒的tick数
uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

//得到现在的时间
uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

//延迟时间
void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time)
        ;
    return;
}

void timer_check()
{
    disable_preempt();
    list_node_t *ptr = sleep_queue.next;
    //队列非空就遍历寻找到时间的点
    while (!list_empty(&sleep_queue) && (ptr != &sleep_queue))
    {

        list_node_t *sleep_list = get_head_node(&sleep_queue);
        pcb_t *temp = (pcb_t *)((reg_t *)sleep_list - 3);
        if (get_timer() >= temp->timer.timeout)
            do_unblock(&temp->list);
        ptr = ptr->next;
    }
    enable_preempt();
}
