#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <type.h>
LIST_HEAD(timers);
uint64_t time_elapsed = 0;
uint32_t time_base = 0;
uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}


uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void timer_check(){
    
}
 
void timer_create(uint64_t tick){
    /*创建一个时钟，时间到之后将其取出*/
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    current_running->sleep_core_id = get_current_cpu_id();
    current_running->end_time = get_ticks() + tick*time_base;
    /*时间到*/
    list_add(&current_running->list,&timers);
}

void check_sleeping(){
    list_node_t *head = &timers;
    /*get the timer*/
    while(!Is_empty_list(&timers)){
        head = head->prev;
        if(head == &timers) break;
        pcb_t *pre_timer = get_entry_from_list(head,pcb_t,list);
        if(get_ticks() >= pre_timer->end_time && pre_timer->sleep_core_id == get_current_cpu_id()){
            list_node_t *unb = head;
            head = unb->prev;
            do_unblock(unb);
        }      
    }
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

