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
    .status = TASK_READY,
    .origin = -1}; // master核
const ptr_t pid0_stack_s = INIT_KERNEL_STACK_SLAVE + 0x0e70;
pcb_t pid0_pcb_slave = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .preempt_count = 0,
    .pgdir = PGDIR_PA,
    .cursor_x = 1,
    .cursor_y = 1,
    .status = TASK_READY,
    .origin = -1}; // slave核

LIST_HEAD(ready_queue);
/*僵尸进程队列*/
LIST_HEAD(ZOMBIE_queue);
/* current running task PCB */
pcb_t *volatile current_running;
pcb_t *volatile prev_running;
/* global process id */
pid_t process_id = 1;

list_node_t *get_process_for_core(int cpu_id)
{
    if (cpu_id == 0)
    {
        list_node_t *find;
        list_node_t *index;
        index = ready_queue.prev;
        while (index != &ready_queue)
        {
            pcb_t *pre = list_entry(index, pcb_t, list);
            if (pre->mask == 1 || pre->mask == 3)
                return index;
            index = index->prev;
        }
    }
    else if (cpu_id != 0)
    {

        list_node_t *find;
        list_node_t *index;
        index = ready_queue.prev;
        while (index != &ready_queue)
        {
            pcb_t *pre = list_entry(index, pcb_t, list);
            if (pre->mask == 2 || pre->mask == 3)
                return index;
            index = index->prev;
        }
    }
    return &ready_queue;
}
/* 回收物理页 */
void recycle(pcb_t *recy_pcb)
{
    long alloc_addr[MAX_PAGE_NUM] = {0};
    if (recy_pcb->pge_h != NULL)
    {
        list_node_t *alloc_list = recy_pcb->pge_h;
        do
        {
            page_t *alloc_page = list_entry(alloc_list, page_t, list);
            int flag = 0;
            for (flag = 0; flag < MAX_PAGE_NUM; flag++)
            {
                if (alloc_page->kva == alloc_addr[flag])
                {
                    break;
                }
            }
            if (flag < MAX_PAGE_NUM)
                continue;
            else
            {
                int i;
                for (i = 0; i < MAX_PAGE_NUM; i++)
                {
                    if (alloc_addr[i] == 0)
                    {
                        alloc_addr[i] = alloc_page->kva;
                        freePage(alloc_page->kva);
                        break;
                    }
                }
                if (i >= MAX_PAGE_NUM)
                {
                    break;
                }
            }
        } while (alloc_list != recy_pcb->pge_h);
    }
}

void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    // check_sleeping();
    int cpu_id = get_current_cpu_id();
    current_running = cpu_id == 0 ? current_running_master : current_running_slave;
    prev_running = current_running;
    if (current_running->status == TASK_RUNNING && current_running->pid)
    {
        current_running->status = TASK_READY;
        list_add(&current_running->list, &ready_queue);
    }

    if (!list_empty(&ready_queue))
    {
        list_node_t *node;
        if ((node = get_process_for_core(cpu_id)) != &ready_queue)
        {
            list_del(node);
            current_running = list_entry(node, pcb_t, list);
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
    /* 如果是同一个进程创建的线程则可以不进行页表的更新 */
    if ((current_running->origin == prev_running->origin && prev_running->origin != -1) || current_running->origin == prev_running->pid)
        ;
    else
    {
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
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    current_running->status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout, <time.h>
    timer_create(sleep_time);
    // 3. reschedule because the current_running is blocked.
    // list_add(&current_running->list,&ready_queue);
    // latency(sleep_time);
    do_scheduler();
}

pid_t do_getpid()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    return (pid_t)current_running->pid;
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    list_add(pcb_node, queue);
    current_running->status = TASK_BLOCKED;
    do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    list_del(pcb_node);
    list_entry(pcb_node, pcb_t, list)->status = TASK_READY;
    list_add(pcb_node, &ready_queue);
}

void do_ps()
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    prints("[PROCESS TABLE]\n");
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid)
        {
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
                prints("PID %d: STATUS: %s MASK = %d\n", pcb[i].pid, "BLOCKED", pcb[i].mask);
                break;
            case TASK_EXITED:
                prints("PID %d: STATUS: %s MASK = %d\n", pcb[i].pid, "EXITED", pcb[i].mask);
                break;
            case TASK_READY:
                prints("PID %d: STATUS: %s MASK = %d\n", pcb[i].pid, "READY", pcb[i].mask);
                break;
            }
        }
    }
}
int find_pcb()
{
    int i;
    for (i = 1; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid == 0)
            return i;
    }
    return -1;
}

int find_name_process(char *name)
{
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid != 0 && !kstrcmp(name, pcb[i].name))
        {
            return TRUE;
        }
    }
    return FALSE;
}

pid_t do_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode)
{
    int pid = find_pcb();
    if (pid == -1)
    {
        prints("[ERROR] : no more resource\n");
        return -1;
    }
    uint32_t elf_id;
    for (elf_id = 0; elf_id < ELF_FILE_NUM; elf_id++)
    {
        if (!kstrcmp(file_name, elf_files[elf_id].file_name))
        {
            break;
        }
    }
    if (elf_id >= ELF_FILE_NUM)
    {
        prints("> [ERROR] : failed to find process %s\n", file_name);
        return -1;
    }

    pcb[pid].pid = pid + 1;
    //分配页表
    pcb[pid].pgdir = allocPage() - PAGE_SIZE;
    pcb[pid].kernel_stack_base = allocPage();
    pcb[pid].user_stack_base = USER_STACK_ADDR;
    //建立映射
    share_pgtable(pcb[pid].pgdir, pa2kva(PGDIR_PA));
    alloc_page_helper(pcb[pid].user_stack_base - PAGE_SIZE, pcb[pid].pgdir);
    pcb[pid].kernel_sp = pcb[pid].kernel_stack_base;
    pcb[pid].user_sp = pcb[pid].user_stack_base;
    pcb[pid].status = TASK_READY;
    pcb[pid].type = USER_PROCESS;
    pcb[pid].mask = 3;
    pcb[pid].mode = mode;
    pcb[pid].cursor_x = 1;
    pcb[pid].cursor_y = 1;
    pcb[pid].name = (char *)kmalloc(kstrlen(elf_files[elf_id].file_name) * sizeof(char) + 1);
    kstrcpy(pcb[pid].name, elf_files[elf_id].file_name);
    ptr_t entry_point = load_elf(elf_files[elf_id].file_content,
                                 *elf_files[elf_id].file_length,
                                 pcb[pid].pgdir,
                                 alloc_page_helper);
    /* get the base of the argv */
    uintptr_t new_argv = pcb[pid].user_stack_base - 0xc0;
    //预留0x40即8个指针位，可供后方的八个0x10大小的argv[i]
    uintptr_t new_argv_pointer = pcb[pid].user_stack_base - 0x100;
    uintptr_t kargv = get_kva_of(new_argv, pcb[pid].pgdir);
    uintptr_t kargv_pointer = get_kva_of(new_argv_pointer, pcb[pid].pgdir);
    for (int j = 0; j < argc; j++)
    {
        /* 指针里面再存放了一个指针,即对应的argv的地址 */
        *((uintptr_t *)kargv_pointer + j) = (uint64_t)new_argv;
        kmemcpy((char *)kargv, argv[j], kstrlen(argv[j]) + 1);
        new_argv = new_argv + kstrlen(argv[j]) + 1;
        kargv = kargv + kstrlen(argv[j]) + 1;
    }
    init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp - 0x100, entry_point, &pcb[pid], argc, new_argv_pointer);
    list_add(&pcb[pid].list, &ready_queue);
    return pcb[pid].pid;
}

void check_wait_queue(int pid)
{
    int i;
    for (i = 0; i < NUM_MAX_TASK; i++)
        if (pcb[i].pid == pid)
            break;
    if (i >= NUM_MAX_TASK)
    {
        prints("[ERROR] : process_%d not exist\n", pid);
        return 0;
    }

    while (!list_empty(&pcb[i].wait_list))
    {
        pcb_t *wait_pcb;
        wait_pcb = list_entry(pcb[i].wait_list.prev, pcb_t, list);
        if (wait_pcb->status != TASK_EXITED)
            do_unblock(pcb[i].wait_list.prev);
    }
}

/* 通用的删除清空pcb操作 */
void kill_pcb(pcb_t *del_pcb)
{
    list_del(&del_pcb->list);
    while (!list_empty(&del_pcb->wait_list))
    {
        pcb_t *wait_pcb;
        wait_pcb = list_entry(del_pcb->wait_list.prev, pcb_t, list);
        if (wait_pcb->status != TASK_EXITED)
            do_unblock(del_pcb->wait_list.prev);
    }
    /*释放它持有的锁*/
    for (int i = 0; i < MAX_LOCK_NUM; i++)
    {
        if (del_pcb->handle_mutex[i] != -1)
        {
            int id = del_pcb->handle_mutex[i]; //该进程持有的锁的id号
            if (!list_empty(&user_lock_array[id].kernel_lock.block_queue))
            {
                list_node_t *node = user_lock_array[id].kernel_lock.block_queue.prev;
                pcb_t *handle_next = list_entry(node, pcb_t, list);
                for (int i = 0; i < MAX_LOCK_NUM; i++)
                {
                    if (handle_next->handle_mutex[i] == -1)
                    {
                        handle_next->handle_mutex[i] = id; //判定该用户持有锁
                        break;
                    }
                }
                do_unblock(user_lock_array[id].kernel_lock.block_queue.prev);
            }
            else
            {
                user_lock_array[id].kernel_lock.lock.status = UNLOCKED; //解锁
                user_lock_array[id].status = UNSED;                     //锁的状态为未使用
                user_lock_array[id].key = -1;
            }
            del_pcb->handle_mutex[i] = -1; //该用户不再持有锁
        }
    }

    if (del_pcb->mode == ENTER_ZOMBIE_ON_EXIT)
        check_wait_queue(del_pcb->pid);

    /* 回收分配的物理页 */
    recycle(del_pcb);

    del_pcb->pge_h = NULL;
    del_pcb->pge_num = 0;
    del_pcb->origin = -1;
    /*回收内核栈*/
    freePage(del_pcb->kernel_stack_base - PAGE_SIZE);
    /*回收用户栈*/
    freePage(get_pfn_of(del_pcb->user_stack_base - PAGE_SIZE, del_pcb->pgdir));

    del_pcb->status = TASK_EXITED;
    del_pcb->pid = 0;
    if (current_running == del_pcb)
        do_scheduler();
}

int do_kill(pid_t pid)
{
    int i;
    /*找到他的pcb结构*/
    for (i = 0; i < NUM_MAX_TASK; i++)
        if (pcb[i].pid == pid)
            break;
    if (i >= NUM_MAX_TASK)
    {
        // prints("[ERROR] process_%d is not exist\n", pid);
        return 0;
    }
    pcb_t *del_pcb = &pcb[i];
    /* 杀死子线程 */

    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid != 0 && pcb[i].origin == del_pcb->pid)
        {
            kill_pcb(&pcb[i]);
        }
    }

    kill_pcb(del_pcb);
    prints("process[%d] has been kill successfuly!\n", pid);

    return pid;
}

void do_exit(void)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;

    /*检查等待队列*/
    if (current_running->mode == ENTER_ZOMBIE_ON_EXIT)
    {
        check_wait_queue(current_running->pid);
    }

    current_running->pid = 0;
    current_running->status = TASK_EXITED;
    current_running->pge_h = NULL;
    current_running->pge_num = 0;

    /*回收内核栈*/
    freePage(current_running->kernel_stack_base - PAGE_SIZE);
    /*回收用户栈*/
    freePage(get_pfn_of(current_running->user_stack_base - PAGE_SIZE, current_running->pgdir));

    do_scheduler();
}

int do_waitpid(pid_t pid)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int i;
    for (i = 0; i < NUM_MAX_TASK; i++)
        if (pcb[i].pid == pid)
            break;
    if (i >= NUM_MAX_TASK)
    {
        prints("[ERROR] : process_%d not exist\n", pid);
        return 0;
    }
    pcb_t *wait_pcb = current_running;
    /*阻塞进程，前提是该进程没有退出并且没有僵尸态*/
    if (pcb[i].status != TASK_EXITED && pcb[i].status != TASK_ZOMBIE)
        do_block(&wait_pcb->list, &pcb[i].wait_list);
    return i;
}

void do_ls()
{
    prints("[TASK TABLE]: \n");
    for (int i = 0; i < ELF_FILE_NUM; i++)
    {
        prints("%s  ", elf_files[i].file_name);
    }
    prints("\n");
}

/* 创建一个线程 */
int do_thread_create(void (*start_routine)(void *), void *arg)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint32_t pid = find_pcb();
    if (pid == -1)
    {
        prints("[ERROR] : no more resource\n");
        return -1;
    }
    pcb[pid].pid = process_id++;
    /* alloc the first_level page */
    /* 使用进程页表 */
    pcb[pid].pgdir = current_running->pgdir;
    pcb[pid].kernel_stack_base = allocPage();               // a kernel virtual addr, has been mapped
    pcb[pid].user_stack_base = USER_STACK_ADDR - PAGE_SIZE; // a user virtual addr, not mapped
    /* map the user_stack */
    /* copy pge list*/
    copy_pge(pcb[pid].pge_h, current_running->pge_h);

    /* 为已经存在的虚拟地址分配 */
    alloc_page_helper(pcb[pid].user_stack_base - PAGE_SIZE, pcb[pid].pgdir);
    pcb[pid].kernel_sp = pcb[pid].kernel_stack_base;
    pcb[pid].user_sp = pcb[pid].user_stack_base;
    pcb[pid].status = TASK_READY;
    pcb[pid].type = USER_PROCESS;
    pcb[pid].mask = 3;
    pcb[pid].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[pid].cursor_x = 1;
    pcb[pid].cursor_y = 1;
    /* copy the name */
    char n[] = "son";
    pcb[pid].name = (char *)kmalloc((kstrlen(n) + kstrlen(current_running->name) + 1) * sizeof(char));
    kstrcpy(pcb[pid].name, current_running->name);
    pcb[pid].name = kstrcat(pcb[pid].name, n);
    /* 进程来源 */
    pcb[pid].origin = current_running->pid;
    ptr_t entry_point = (ptr_t)start_routine;
    init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp, entry_point, &pcb[pid], (void *)arg, NULL);
    /* 利用父进程的GP寄存器 */
    pcb[pid].save_context->regs[3] = current_running->save_context->regs[3];
    list_add(&pcb[pid].list, &ready_queue);
    return pcb[pid].pid;
}
/* 等待线程执行完毕 */
int do_thread_join(int thread)
{
    /*挂起等待*/
    do_waitpid(thread);
    return 0;
}

/* close pte W */
void close_pte_W(pcb_t *cw_pcb)
{
    if (cw_pcb->pge_h != NULL)
    {
        list_node_t *pointer = cw_pcb->pge_h;
        do
        {
            page_t *pre = list_entry(pointer, page_t, list);
            /* close W */
            (*pre->phyc_page_pte) &= ~(_PAGE_WRITE);
            pointer = pointer->next;
        } while (pointer != cw_pcb->pge_h);
    }
}

/* copy pge */
void copy_pge(list_head *des, list_head *scr)
{
    list_node_t *scr_pointer = scr;
    if (scr == NULL)
        return;
    do
    {
        page_t *des_page = (page_t *)kmalloc(sizeof(page_t));
        page_t *src_page = list_entry(scr_pointer, page_t, list);
        des_page->kva = src_page->kva;
        des_page->vta = src_page->vta;
        des_page->phyc_page_pte = src_page->phyc_page_pte;
        des_page->SD_place = src_page->SD_place;
        if (des == NULL)
            des = &des_page->list;
        else
            list_add(&des_page->list, des);
        scr_pointer = scr_pointer->next;
    } while (scr_pointer != scr);
}
