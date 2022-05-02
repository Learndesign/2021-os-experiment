#include <os/list.h>
#include <screen.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <os/string.h>
// #include <pgtable.h>
#define PRIORIT_BASE 4
#define TIME_BASE 1
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_MSTER + 0x0e70;
pcb_t pid0_pcb_master = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .preempt_count = 0,
    .pgdir = PGDIR_PA,
    .cursor_x = 1,
    .cursor_y = 1,
    .cursor_y_base = 0,    
    .status   = TASK_READY,
    .produce = -1,
    .used = 1
};  //master核
const ptr_t pid0_stack_s = INIT_KERNEL_STACK_SLAVE + 0x0e70;
pcb_t pid0_pcb_slave = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .preempt_count = 0,
    .pgdir = PGDIR_PA,
    .cursor_x = 1,
    .cursor_y = 1,
    .cursor_y_base = 0,
    .status   = TASK_READY,
    .produce  = -1,
    .used = 1
};  //slave核

LIST_HEAD(ready_queue);
/*僵尸进程队列*/
LIST_HEAD(ZOMBIE_queue);
/* current running task PCB */
pcb_t * volatile current_running;
pcb_t * volatile prev_running;
list_head net_recv_queue;
list_head net_send_queue;
/* global process id */
pid_t process_id = 1;

/* 优先级调度 */
pcb_t * get_higher_process(list_head * queue, uint64_t schedler_time){  
    int score = 0;
    list_node_t * pre;
    pcb_t * get_pcb;
    for(pre = queue->prev; pre!=queue && !Is_empty_list(queue) ; pre=pre->prev ){
        pcb_t * check = get_entry_from_list(pre,pcb_t,list);
        /* 优先级由优先级和调度次数决定，即一个进程如果等太久他也能跑，总有等的太久的进程
         * 通过调节优先级（PRIORIT_BASE）和等待时间（TIME_BASE）的占比的比重
         * 可以得到评分，随后根据评分来操作
         * 调度时间差需要除以6000即大概等待
         * */
        int get_score = check->priority * PRIORIT_BASE + ((schedler_time-check->pre_time) / 6000)*TIME_BASE;
        if(get_score > score){
            get_pcb = check;
            score = get_score;
        } 
    }
    return get_pcb;
}

list_node_t * get_process_for_core(int cpu_id){
    if (cpu_id == 0){
        list_node_t * find;
        list_node_t * index;
        index = ready_queue.prev;
        while(index != &ready_queue){
            pcb_t * pre = get_entry_from_list(index, pcb_t, list);
            if(pre->mask == 1 || pre->mask == 3)
                return index;
            index = index->prev;
        }
    } else if (cpu_id != 0){

        list_node_t * find;
        list_node_t * index;
        index = ready_queue.prev;
        while(index != &ready_queue){
            pcb_t * pre = get_entry_from_list(index, pcb_t, list);
            if(pre->mask == 2 || pre->mask == 3)
                return index;
            index = index->prev;
        }
    }
    return &ready_queue;
}
/* 回收物理页 */
void recycle(pcb_t * recy_pcb){
    long alloc_addr[MAX_PAGE_NUM] = {0};
    if(recy_pcb->pge_h != NULL){
        list_node_t * alloc_list = recy_pcb->pge_h;
        do{
            page_t * alloc_page = get_entry_from_list(alloc_list, page_t, list);
            int flag = 0;
            for (flag = 0; flag < MAX_PAGE_NUM; flag++){
                if (get_pfn_of(alloc_page->vta, recy_pcb->pgdir) == alloc_addr[flag]){
                    break;
                }                     
            }
            if(flag < MAX_PAGE_NUM) 
                continue;
            else{
                int i;
                for (i = 0; i < MAX_PAGE_NUM; i++){
                    if(alloc_addr[i] == 0){
                        alloc_addr[i] = get_pfn_of(alloc_page->vta, recy_pcb->pgdir);
                        freePage(alloc_addr[i]);
                        break;
                    }
                }
                if(i >= MAX_PAGE_NUM){
                    break;
                }
            }
        } while (alloc_list != recy_pcb->pge_h);
    }    
}


/* 通用的删除清空pcb操作 */
void kill_pcb(pcb_t * del_pcb){
    list_del_point(&del_pcb->list);
    while(!Is_empty_list(&del_pcb->wait_list)){
        pcb_t * wait_pcb;
        wait_pcb = get_entry_from_list(del_pcb->wait_list.prev,pcb_t,list);
        if(wait_pcb->status != TASK_EXITED)
            do_unblock(del_pcb->wait_list.prev);
    }
    /*释放它持有的锁*/
    for (int i = 0; i < MAX_LOCK_NUM; i++){
        if(del_pcb->handle_mutex[i]!=-1){
            int id = del_pcb->handle_mutex[i];//该进程持有的锁的id号
            if(!Is_empty_list(&User_Lock[id].kernel_lock.block_queue)){
                list_node_t * node = User_Lock[id].kernel_lock.block_queue.prev;
                pcb_t * handle_next = get_entry_from_list(node, pcb_t, list);
                for (int i = 0; i < MAX_LOCK_NUM; i++){
                    if(handle_next->handle_mutex[i] == -1){
                        handle_next->handle_mutex[i] = id;//判定该用户持有锁
                        break;
                    }
                }
                do_unblock(User_Lock[id].kernel_lock.block_queue.prev);                
            }else{
                User_Lock[id].kernel_lock.lock.status = UNLOCKED;//解锁
                User_Lock[id].status = UNSED;//锁的状态为未使用
                User_Lock[id].key = -1;
            }
            del_pcb->handle_mutex[i] = -1;//该用户不再持有锁
        }
    }
    
    /* 回收分配的物理页 */
    /* 如果的确有物理页分配 */
    // recycle(del_pcb);
    
    del_pcb->pge_h = NULL;
    del_pcb->pge_num = 0;
    del_pcb->produce = -1;
    del_pcb->fork = -1;
    del_pcb->fork_son = -1;
    /*回收内核栈*/
    freePage(del_pcb->kernel_stack_base - PAGE_SIZE);
    /*回收用户栈*/
    freePage(get_pfn_of(del_pcb->user_stack_base - PAGE_SIZE, del_pcb->pgdir));

    del_pcb->status = TASK_EXITED;
    del_pcb->used = 0;
    if(current_running == del_pcb)
        do_scheduler();
}

void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    // check_sleeping();
    int cpu_id = get_current_cpu_id();
    current_running = cpu_id == 0 ? current_running_master : current_running_slave; 
    prev_running = current_running;  
    if(current_running->status == TASK_RUNNING && current_running->pid){
        current_running->status = TASK_READY;
        list_add(&current_running->list,&ready_queue);
    }
    /*优先级调度*/
    #ifdef PRIORITY
        uint64_t now = get_ticks();
        current_running->pre_time = now; 
        current_running = get_higher_process(&ready_queue,now);
        list_del_point(&current_running->list);
    #endif
    #ifdef NO_PRIORITY
        if(!Is_empty_list(&ready_queue)){ 
            list_node_t * node;
            if ((node = get_process_for_core(cpu_id)) != &ready_queue){ 
                list_del_point(node);
                current_running = get_entry_from_list(node, pcb_t, list);
            }else{
                current_running = cpu_id == 0 ? &pid0_pcb_master : &pid0_pcb_slave;
            }
            if(cpu_id == 0)
                current_running_master = current_running;//更换
            else
                current_running_slave  = current_running;//更换
        }else{
            if(current_running->status != TASK_RUNNING){
                current_running = cpu_id == 0 ? &pid0_pcb_master : &pid0_pcb_slave;
                if(cpu_id == 0)
                    current_running_master = current_running;//更换
                else
                    current_running_slave  = current_running;//更换
            }
        }
    #endif
    /* 如果是同一个进程创建的线程则可以不进行页表的更新 */
    if((current_running->produce == prev_running->produce && prev_running->produce != -1)
        || current_running->produce == prev_running->pid) ;
    else{
        set_satp(SATP_MODE_SV39, current_running->pid, (uint64_t)kva2pa((current_running->pgdir)) >> 12);
        local_flush_tlb_all();
    }
    // TODO: switch_to current_running
    current_running->status = TASK_RUNNING;
    switch_to(prev_running, current_running);  
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    current_running->status=TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout, <time.h>
    timer_create(sleep_time);
    // 3. reschedule because the current_running is blocked.
    // list_add(&current_running->list,&ready_queue);
    // latency(sleep_time);   
    do_scheduler();

}

pid_t do_getpid(){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    return (pid_t)current_running->pid;
}

int do_get_id(){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    return current_running->pid;
}

int do_get_priorit(){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    return current_running->priority;
}

void do_priori(int priori, int pcb_id){
    if(pcb_id){
        pcb[pcb_id].priority = priori;
        pcb[pcb_id].status = TASK_READY;
        pcb[pcb_id].pre_time = get_ticks() ;
        list_add(&pcb[pcb_id].list, &ready_queue);
    }      
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    list_add(pcb_node,queue);
    current_running->status = TASK_BLOCKED;
    do_scheduler();//因为被阻塞，需要切换到其他的任务，因此涉及到任务切换
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    list_del_point(pcb_node);//在队列中删除指定元素
    get_entry_from_list(pcb_node,pcb_t,list)->status=TASK_READY;//切换为准备状态
    list_add(pcb_node, &ready_queue);
}

void do_ps(){
    prints(">-------[PROCESS TABLE]---------\n");
     for (int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].used){
            switch(pcb[i].status){
                case TASK_RUNNING:
                    if (current_running_master == &pcb[i] && get_current_cpu_id() == 0)
                        prints("PID %d: %s MASK = %d RUNNING (core 0) name: %s \n", pcb[i].pid, "RUNNING", pcb[i].mask, pcb[i].name);
                    if (current_running_slave == &pcb[i] && get_current_cpu_id() == 1)
                        prints("PID %d: %s MASK = %d RUNNING (core 1) name: %s \n", pcb[i].pid, "RUNNING", pcb[i].mask, pcb[i].name);
                    if (current_running != &pcb[i] && get_current_cpu_id() == 0)
                        prints("PID %d: %s MASK = %d RUNNING (core 1) name: %s \n", pcb[i].pid, "RUNNING", pcb[i].mask, pcb[i].name);
                    if (current_running != &pcb[i] && get_current_cpu_id() == 1)
                        prints("PID %d: %s MASK = %d RUNNING (core 0) name: %s \n", pcb[i].pid, "RUNNING", pcb[i].mask, pcb[i].name);
                    break;
                case TASK_BLOCKED:
                    prints("PID %d: %s MASK = %d name: %s \n", pcb[i].pid, "BLOCKED", pcb[i].mask, pcb[i].name);
                    break;
                case TASK_EXITED:
                    prints("PID %d: %s MASK = %d name: %s \n", pcb[i].pid, "BLOCKED", pcb[i].mask, pcb[i].name);
                    break;
                case TASK_READY:
                    prints("PID %d: %s MASK = %d name: %s \n", pcb[i].pid, "BLOCKED", pcb[i].mask, pcb[i].name);
                    break;
                case TASK_ZOMBIE:
                    prints("PID %d: %s MASK = %d name: %s \n", pcb[i].pid, "BLOCKED", pcb[i].mask, pcb[i].name);
                    break;
            }
        }
    }
}
int find_pcb(){
    int i;
    for(i=1; i<NUM_MAX_TASK; i++){
        if(pcb[i].used == 0 /*|| pcb[i].status == TASK_EXITED*/)
            return i;
    }
    return -1;
}

int find_name_process(char * name){
    for (int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].used && !kstrcmp(name, pcb[i].name)){
            return TRUE;
        }
    }
    return FALSE;
}

pid_t do_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    int pid = find_pcb();
    if(pid == -1){
        prints("[Error] no more resource\n");
        return -1;
    }   
    uint32_t elf_id;
    for (elf_id = 0; elf_id < ELF_FILE_NUM; elf_id++){
        if (!kstrcmp(file_name, elf_files[elf_id].file_name)){
            break;
        }       
    }
    if(elf_id >= ELF_FILE_NUM){
        prints("> [Error] failed to find process %s\n", file_name);
        return -1;
    }
    /* maybe allow to creat one process for one more */
    // else{
    //     if(find_name_process(file_name)){
    //         prints("> [Error] the process: %s has been created already!\n",file_name);
    //         return -1;
    //     }
    // }
    
    pcb[pid].pre_time = get_ticks();
    pcb[pid].pid = process_id++;
    /* alloc the first_level page */
    pcb[pid].pgdir = allocPage() - PAGE_SIZE;   
    pcb[pid].kernel_stack_base = allocPage();         //a kernel virtual addr, has been mapped
    pcb[pid].user_stack_base = USER_STACK_ADDR;       //a user virtual addr, not mapped
    share_pgtable(pcb[pid].pgdir, pa2kva(PGDIR_PA));
    /* map the user_stack */
    // alloc_page_helper(pcb[pid].kernel_stack_base - PAGE_SIZE, pcb[pid].pgdir, MAP_USER);
    alloc_page_helper(pcb[pid].user_stack_base - PAGE_SIZE, pcb[pid].pgdir);
    pcb[pid].used = 1;
    pcb[pid].kernel_sp = pcb[pid].kernel_stack_base;
    pcb[pid].user_sp = pcb[pid].user_stack_base;
    pcb[pid].status = TASK_READY;
    pcb[pid].type = USER_PROCESS;
    pcb[pid].mask = 3;
    pcb[pid].mode = mode;
    pcb[pid].cursor_x= 1;
    pcb[pid].cursor_y= 1;
    pcb[pid].cursor_y_base = 0;
    pcb[pid].fork = -1;
    pcb[pid].produce = -1;
    /* copy the name */
    pcb[pid].name = (char *) kmalloc(kstrlen(elf_files[elf_id].file_name)*sizeof(char) + 1);
    kstrcpy(pcb[pid].name, elf_files[elf_id].file_name);       
    ptr_t entry_point = load_elf(elf_files[elf_id].file_content,
                                *elf_files[elf_id].file_length, 
                                 pcb[pid].pgdir, 
                                 alloc_page_helper);    
    /* get the base of the argv */
    uintptr_t new_argv = pcb[pid].user_stack_base - 0xc0;
    uintptr_t new_argv_pointer = pcb[pid].user_stack_base - 0x100;//预留0x40即8个指针位，可供后方的八个0x10大小的argv[i]    
    uintptr_t kargv = get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    for (int j = 0; j < argc; j++){ 
        /* 指针里面再存放了一个指针,即对应的argv的地址 */
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kmemcpy((char *)kargv , argv[j], kstrlen(argv[j]) + 1);
        new_argv = new_argv + kstrlen(argv[j]) + 1;
        kargv = kargv + kstrlen(argv[j]) + 1;
    }    
    init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp - 0x100, entry_point, &pcb[pid], argc, new_argv_pointer);//(void *)(pcb[pid].user_stack_base - 0x100));
    list_add(&pcb[pid].list,&ready_queue);
    return pcb[pid].pid;
}

/*强制杀死一个进程,杀死以后直接释放所有资源*/
extern int do_kill(char *name){
    int pid;
    if(!kstrcmp(name, "shell")){
        prints("> Warning: don't try to kill the shell!\n");
        return -1;
    }
    for(pid = 0; pid < NUM_MAX_TASK; pid ++){
        if(pcb[pid].used == 1 && !kstrcmp(pcb[pid].name, name)){     
            break;
        }          
    }
    if(pid >= NUM_MAX_TASK){
        prints("> [Error] falied to find the process: %s\n", name);
        return -1;
    }
    /* 杀死子线程 */
    pcb_t * del_pcb = &pcb[pid];

    for (int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].used == 1 && pcb[i].produce == del_pcb->pid){
            /**/
            kill_pcb(&pcb[i]);
            /**/
        }
    }

    /**/
    kill_pcb(del_pcb);
    /**/

    return del_pcb->pid;    
}


int do_waitpid(pid_t pid){
    int i;
    /*找到他的pcb结构*/
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    for(i=0; i<NUM_MAX_TASK; i++)
        if(pcb[i].pid == pid)
            break;
    if(i>=NUM_MAX_TASK){
        prints("[Error] no this pid process\n");
        return 0;
    }
    pcb_t * wait_pcb = current_running;
    /*阻塞进程，前提是该进程没有退出并且没有僵尸态*/
    if(pcb[i].status != TASK_EXITED && pcb[i].status != TASK_ZOMBIE)
        do_block(&wait_pcb->list,&pcb[i].wait_list);
    return i;
}

/*退出当前任务,是否需要回收pcb?*/
void do_exit(void){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave; 
    pcb_t * exit_pcb;
    exit_pcb = current_running;
    list_node_t * wt_list;
    /*释放所有正在等待他的的进程*/
    while(!Is_empty_list(&exit_pcb->wait_list)){
        pcb_t * wait_pcb;
        wait_pcb = get_entry_from_list(exit_pcb->wait_list.prev,pcb_t,list);
        if(wait_pcb->status != TASK_EXITED)
            do_unblock(exit_pcb->wait_list.prev);
    }
    /*进入僵尸态*/
    // if(exit_pcb->mode == ENTER_ZOMBIE_ON_EXIT){
    //     list_add(&exit_pcb->list, &ZOMBIE_queue);
    //     exit_pcb->status = TASK_ZOMBIE;
    // }        
    // else{
    exit_pcb->used = 0;
    exit_pcb->status = TASK_EXITED;
    // }
    /*释放它持有的锁*/
    /*回收物理页*/
    // recycle(exit_pcb);

    exit_pcb->pge_h = NULL;
    exit_pcb->pge_num = 0;
    exit_pcb->produce = -1;
    exit_pcb->fork = -1;
    exit_pcb->fork_son = -1;
    /*回收内核栈*/
    freePage(exit_pcb->kernel_stack_base - PAGE_SIZE);
    /*回收用户栈*/
    freePage(get_pfn_of(exit_pcb->user_stack_base - PAGE_SIZE, exit_pcb->pgdir));

    do_scheduler();
}
/* 设置MASK */
int do_taskset(task_info_t *task, int mask, int pid, int mode){
    if(mode == 0){
        int pid = find_pcb();
        if(pid == -1){
            prints("[Error] no more resource\n");
            return -1;
        }
        /*标记为使用*/
        pcb[pid].used = 1;
        pcb[pid].mask = mask;
        pcb[pid].kernel_sp = pcb[pid].kernel_stack_base;
        pcb[pid].user_sp = pcb[pid].user_stack_base;
        pcb[pid].mode = mode;
        pcb[pid].pid = process_id++;//id不重要
        pcb[pid].status = TASK_READY;//设置为准备状态
        pcb[pid].type = task->type;//用户进程
        pcb[pid].cursor_x= 0;
        pcb[pid].cursor_y= 0;
        //初始化栈
        init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp, task->entry_point, &pcb[pid], NULL, NULL);
        //是否需要拉链？
        list_add(&pcb[pid].list,&ready_queue);      
        return pcb[pid].pid;  
    }else{
        int i;
        for(i=0; i<NUM_MAX_TASK; i++)
            if(pcb[i].pid == pid)
                break;
        if(i>=NUM_MAX_TASK){
            prints("[Error] no this pid process\n");
            return 0;
        }     
        pcb[i].mask = mask;  
    }   
    return 0;
}
/* P4 */
// void do_ls(){
//     prints("> Start: \n");
//     for (int i = 0; i < ELF_FILE_NUM; i++){
//         prints("%s ",elf_files[i].file_name);
//     }
//     prints("\n");
// }

/* 创建一个线程 */
int do_thread_creat( void (*start_routine)(void*), void *arg){
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave;  
    uint32_t pid = find_pcb();
    if(pid == -1){
        prints("[Error] no more resource\n");
        return -1;
    }       
    pcb[pid].pre_time = get_ticks();
    pcb[pid].pid = process_id++;
    /* alloc the first_level page */
    /* 复用页表 */
    pcb[pid].pgdir = current_running->pgdir; //allocPage() - PAGE_SIZE;   
    pcb[pid].kernel_stack_base = allocPage();         //a kernel virtual addr, has been mapped
    /* 寻找一个没有使用过的虚地址 */
    uintptr_t vta;
new:vta = krand() % 0xf0000000;
    vta &= ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
    if(!check_page_map(vta, current_running->pgdir))
        goto new;    
    pcb[pid].user_stack_base = vta;
    /* map the user_stack */
    /* copy pge list*/
    pcb[pid].pge_h = copy_pge(current_running->pge_h);
    pcb[pid].clock_point = pcb[pid].pge_h;
    /* 为已经存在的虚拟地址分配 */
    alloc_page_helper(pcb[pid].user_stack_base - PAGE_SIZE, pcb[pid].pgdir);
    pcb[pid].used = 1;
    pcb[pid].kernel_sp = pcb[pid].kernel_stack_base;
    pcb[pid].user_sp = pcb[pid].user_stack_base;
    pcb[pid].status = TASK_READY;
    pcb[pid].type = USER_PROCESS;
    pcb[pid].mask = 3;
    pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[pid].cursor_x= 1;
    pcb[pid].cursor_y= 1;
    pcb[pid].cursor_y_base = current_running->cursor_y_base;
    /* copy the name */
    char n[] = "_son";
    pcb[pid].name = (char *)kmalloc((kstrlen(n) + kstrlen(current_running->name) + 1) * sizeof(char));
    kstrcpy(pcb[pid].name, current_running->name);
    pcb[pid].name =  kstrcat(pcb[pid].name, n);
    /* 标识出其生产者 */
    pcb[pid].produce = current_running->pid;
    pcb[pid].fork = -1;    
    ptr_t entry_point = (ptr_t)start_routine;
    init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp, entry_point, &pcb[pid], (void *)arg, NULL);
    /* 利用父进程的GP寄存器 */
    pcb[pid].save_context->regs[3] = current_running->save_context->regs[3]; 
    list_add(&pcb[pid].list,&ready_queue);
    return pcb[pid].pid;
}
/* 等待线程执行完毕 */
int do_thread_join(int thread){
    /*挂起等待*/
    do_waitpid(thread);
    return 0;
}

/* close pte W */
void close_pte_W(pcb_t * cw_pcb){
    if(cw_pcb->pge_h != NULL){
        list_node_t * pointer = cw_pcb->pge_h;
        do{
            page_t * pre = get_entry_from_list(pointer, page_t, list);
            /* close W */
            // get_PTE_of(pre->vta, cw_pcb->pgdir);
            (*get_PTE_of(pre->vta, cw_pcb->pgdir)) &= ~(_PAGE_WRITE);
            pointer = pointer->next;
        } while (pointer != cw_pcb->pge_h);       
    }
}

/* copy pge */
list_head * copy_pge(list_head *scr){
    list_node_t * scr_pointer = scr;
    list_head * des = NULL;
    if(scr == NULL){
        des = scr;
        return des;
    }
        
    do{
        page_t * des_page = (page_t *)kmalloc(sizeof(page_t));
        page_t * src_page = get_entry_from_list(scr_pointer, page_t, list);
        // des_page->kva = src_page->kva;
        des_page->vta = src_page->vta;
        // des_page->phyc_page_pte = src_page->phyc_page_pte;
        des_page->SD_place = src_page->SD_place;
        if(des == NULL){
            des = &des_page->list;
            init_list(des);
        } 
        else
            list_add(&des_page->list, des);
        scr_pointer = scr_pointer->next;
    } while (scr_pointer != scr);
    return des;
}

/* 清空原有的虚拟地址的映射 */
void clear_phyc_map(uintptr_t pgdir,list_head * page){
    list_node_t * pointer = page;
    if(page != NULL){
        do{
            page_t * clear_page = get_entry_from_list(pointer, page_t, list);
            uint64_t vpn2 = (clear_page->vta >> 30) & ~(~0 << 9);  //vpn2
            ((PTE *)pgdir)[vpn2] = 0;
            pointer = pointer->next;
        } while (pointer != page);        
    }
}


/* fork一个进程 */
int do_fork(){
    /**/
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave;  
    uint32_t pid = find_pcb();
    if(pid == -1){
        prints("[Error] no more resource\n");
        return -1;
    }       
    uint32_t elf_id;
    for (elf_id = 0; elf_id < ELF_FILE_NUM; elf_id++){
        if (!kstrcmp(current_running->name, elf_files[elf_id].file_name)){
            break;
        }       
    }
    if(elf_id >= ELF_FILE_NUM){
        prints("> [Error] failed to find process %s\n", current_running->name);
        return -1;
    }
    pcb[pid].pre_time = get_ticks();
    pcb[pid].pid = process_id++;
    /* alloc the first_level page */
    pcb[pid].pgdir = allocPage() - PAGE_SIZE;   
    pcb[pid].kernel_stack_base = allocPage();         //a kernel virtual addr, has been mapped
    pcb[pid].user_stack_base = USER_STACK_ADDR;       //a user virtual addr, not mapped
    share_pgtable(pcb[pid].pgdir, pa2kva(PGDIR_PA));
    re_load_elf(elf_files[elf_id].file_content,
                *elf_files[elf_id].file_length, 
                 pcb[pid].pgdir,
                 current_running->pgdir, 
                 alloc_page_point_phyc);    
    /* close PTE W */
    close_pte_W(current_running);
    /* copy pge list*/
    pcb[pid].pge_h = copy_pge(current_running->pge_h);  


    PTE * fk_user_sp_pte = alloc_page_point_phyc(pcb[pid].user_stack_base - PAGE_SIZE,  pcb[pid].pgdir,
                        get_pfn_of(pcb[pid].user_stack_base - PAGE_SIZE, current_running->pgdir),
                        MAP_USER);
    (*fk_user_sp_pte) &= ~(_PAGE_WRITE);
    PTE * og_user_sp_pte = get_PTE_of(current_running->user_stack_base - PAGE_SIZE, current_running->pgdir);
    (*og_user_sp_pte) &= ~(_PAGE_WRITE);
    /* alloc new stack*/
    
    /* share kernel stack */
    share_pgtable(pcb[pid].kernel_stack_base - PAGE_SIZE, current_running->kernel_stack_base - PAGE_SIZE);
    
    /* remap */
    list_node_t * pointer = pcb[pid].pge_h;
    /* 不同的虚拟地址，同样的物理空间 */    
    if(pcb[pid].pge_h != NULL){
        do{
            page_t * remap_page = get_entry_from_list(pointer, page_t, list);
            PTE * new_pte = alloc_page_point_phyc(remap_page->vta, pcb[pid].pgdir, 
                                                  get_pfn_of(remap_page->vta, current_running->pgdir),
                                                  MAP_USER);
            /* set pte as the father the W has been cleaned*/
            set_attribute(new_pte, get_attribute(*get_PTE_of(remap_page->vta, current_running->pgdir)));
            // remap_page->phyc_page_pte = new_pte;
            pointer = pointer->next;
        } while (pointer != pcb[pid].pge_h);        
    }

    /* has remap all phyc space as the father */
    
    pcb[pid].used = 1;
    pcb[pid].kernel_sp =pcb[pid].kernel_stack_base - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb[pid].user_sp = current_running->user_sp;
    pcb[pid].status = TASK_READY;
    pcb[pid].type = USER_PROCESS;
    pcb[pid].mask = 3;
    pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[pid].cursor_x= 1;
    pcb[pid].cursor_y= 1;
    pcb[pid].cursor_y_base = current_running->cursor_y_base;
    pcb[pid].save_context = (regs_context_t *)(pcb[pid].kernel_stack_base - sizeof(regs_context_t));
    pcb[pid].switch_context = (switchto_context_t *)(pcb[pid].kernel_stack_base - sizeof(regs_context_t) - sizeof(switchto_context_t));
    /* copy the name */

    pcb[pid].switch_context->regs[0] = (reg_t)&ret_from_exception; //
    pcb[pid].switch_context->regs[1] = pcb[pid].kernel_sp        ;
    pcb[pid].save_context->regs[2] = pcb[pid].user_sp            ;
    pcb[pid].save_context->regs[4] = (reg_t)&pcb[pid]            ;
    pcb[pid].save_context->regs[10] = 0                          ;//子进程返回0

    char n[] = "_son";
    pcb[pid].name = (char *)kmalloc((kstrlen(n) + kstrlen(current_running->name) + 1) * sizeof(char));
    kstrcpy(pcb[pid].name, current_running->name);
    pcb[pid].name =  kstrcat(pcb[pid].name, n);
    /* 标识出其生产者 */
    pcb[pid].produce = -1;//current_running->pid;   
    pcb[pid].fork = current_running->pid; 
    current_running->fork_son = pid;
    pcb[pid].pge_num = current_running->pge_num;
    pcb[pid].clock_point = pcb[pid].pge_h;

    init_list(&pcb[pid].wait_list);
    for (int i = 0; i < MAX_LOCK_NUM; i++){
        pcb[pid].handle_mutex[i] = -1;   
    }
    list_add(&pcb[pid].list,&ready_queue);    
    return pcb[pid].pid;
}