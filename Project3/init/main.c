/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/lock.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <test.h>

#include <csr.h>

extern void ret_from_exception();
extern void __global_pointer$();

uint64_t kernel_stack[NUM_MAX_TASK];
uint64_t user_stack[NUM_MAX_TASK];

void init_stack_array()
{

    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        kernel_stack[i] = allocPage(1);
        user_stack[i] = allocPage(1);
    }
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, void *arg)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pcb->save_context = pt_regs;
    for (int i = 0; i < 32; i++)
    {
        pt_regs->regs[i] = 0;
    }
    pt_regs->regs[2] = user_stack;
    pt_regs->regs[3] = (reg_t)__global_pointer$;
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;
    pt_regs->regs[10] = arg; //a0放置参数

    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);

    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;
    pcb->switch_context = st_regs;
    pcb->user_sp = user_stack;
    st_regs->regs[0] = pcb->type == USER_PROCESS || pcb->type == USER_THREAD ? (reg_t)&ret_from_exception : entry_point;
    //st_regs->regs[1] = user_stack;
    st_regs->regs[1] = pcb->kernel_sp;
    for (int i = 2; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
}

int findpid()
{
    int i;
    for (i = 1; i < NUM_MAX_TASK; i++)
    {
        if (pcb[i].pid == 0)
            return i;
    }
    return -1;
}

pid_t do_spawn(task_info_t *task, void *arg, spawn_mode_t mode)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    int i = findpid();
    if (i <= 0)
    {
        printk("> [ERROR] TASK num has full.\n");
        while (1)
            ;
    }
    pcb[i].mask = current_running->mask;
    pcb[i].status = TASK_READY;
    pcb[i].pid = i + 1;
    pcb[i].list.priority = i + 1;
    pcb[i].mode = mode;
    pcb[i].type = task->type;
    pcb[i].cursor_x = 0;
    pcb[i].cursor_y = 0;
    pcb[i].lock_num = 0;
    pcb[i].kernel_sp = kernel_stack[i];
    pcb[i].user_sp = user_stack[i];
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], arg);
    list_add(&pcb[i].list, &ready_queue);
    return pcb[i].pid;
}

static void init_shell()
{
    init_list_head(&ready_queue);
    init_list_head(&sleep_queue);
    init_list_head(&wait_queue);
    pcb[0].mask = 3;
    pcb[0].kernel_sp = kernel_stack[0];
    pcb[0].user_sp = user_stack[0];
    pcb[0].pid = 1;
    pcb[0].list.priority = 1;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    pcb[0].cursor_x = 0;
    pcb[0].cursor_y = 0;
    pcb[0].lock_num = 0;
    void *arg = 0;
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, &test_shell, &pcb[0], arg);
    list_add(&pcb[0].list, &ready_queue);
    current_running_master = &pid0_pcb_master;
    current_running_slave = &pid0_pcb_slave;
}

int do_taskset(task_info_t *task, int mask, int pid, int mode)
{
    if (mode == 0)
    {
        int pid = findpid();
        if (pid <= 0)
        {
            printk("> [ERROR] TASK num has full.\n");
            while (1)
                ;
        }

        pcb[pid].mask = mask;
        pcb[pid].kernel_sp = kernel_stack[pid];
        pcb[pid].user_sp = user_stack[pid];
        pcb[pid].mode = mode;
        pcb[pid].pid = ++process_id;
        pcb[pid].status = TASK_READY;
        pcb[pid].type = task->type;
        pcb[pid].cursor_x = 0;
        pcb[pid].cursor_y = 0;
        //初始化栈
        init_pcb_stack(pcb[pid].kernel_sp, pcb[pid].user_sp, task->entry_point, &pcb[pid], NULL);
        list_add(&pcb[pid].list, &ready_queue);
        return pcb[pid].pid;
    }
    else
    {
        int i;
        for (i = 0; i < NUM_MAX_TASK; i++)
            if (pcb[i].pid == pid)
                break;
        if (i >= NUM_MAX_TASK)
        {
            prints("[ERROR]This pid process not exited\n");
            return 0;
        }
        pcb[i].mask = mask;
    }
    return 0;
}

void syscall_error(void)
{
    printk("> [ERROR]: Syscall not exist \n");
    while (1)
        ;
}

//初始化系统调用的数组
static void init_syscall(void)
{
    // initialize system call table.
    for (int i = 0; i < NUM_SYSCALLS; i++)
        syscall[i] = &syscall_error;
    syscall[SYSCALL_SLEEP] = &do_sleep;

    syscall[SYSCALL_LOCKCREATE] = &lock_create;
    syscall[SYSCALL_LOCKJION] = &lock_jion;
    syscall[SYSCALL_FORK] = &do_fork;
    syscall[SYSCALL_GET_ID] = &do_get_id;
    syscall[SYSCALL_PRIORIT] = &do_priori;
    syscall[SYSCALL_GET_PRIORIT] = &do_get_priorit;
    syscall[SYSCALL_YIELD] = &do_scheduler;

    syscall[SYSCALL_WRITE] = &screen_write;
    syscall[SYSCALL_READ] = &sbi_console_getchar;

    syscall[SYSCALL_CURSOR] = &screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = &screen_reflush;
    syscall[SYSCALL_DELETE] = &screen_delete;

    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK] = &get_ticks;
    syscall[SYSCALL_GET_WALL_TIME] = &get_wall_time;

    syscall[SYSCALL_SCREEN_CLEAR] = &screen_clear;
    syscall[SYSCALL_SPAWN] = &do_spawn;
    syscall[SYSCALL_KILL] = &do_kill;
    syscall[SYSCALL_PS] = &do_process_show;
    syscall[SYSCALL_WAITPID] = &do_waitpid;
    syscall[SYSCALL_EXIT] = &do_exit;

    syscall[SYSCALL_BARRIER_WAIT] = &do_barrier_wait;
    syscall[SYSCALL_BARRIER_INIT] = &user_barrier_create;
    syscall[SYSCALL_SEM_INIT] = &user_sem_create;
    syscall[SYSCALL_SEM_DESTORY] = &user_sem_destory;
    syscall[SYSCALL_SEM_UP] = &user_semaphore_signal;
    syscall[SYSCALL_SEM_DOWN] = &user_semaphore_wait;

    syscall[SYSCALL_MBOX_OPEN] = &do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = &do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = &do_mbox_send;
    syscall[SYACALL_MBOX_RECV] = &do_mbox_recv;
    syscall[SYSCALL_MBOX_SEND_RECV] = &do_mbox_send_recv;

    syscall[SYSCALL_TASKSET] = &do_taskset;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    if (get_current_cpu_id() == 0)
    {
        smp_init();
        // init stack array
        init_stack_array();
        init_shell();
        printk("> [INIT] init shell succeeded.\n\r");

        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // fdt_print(riscv_dtb);

        // init lock_array
        init_user_lock_array();
        printk("> [INIT] USER_LOCK_ARRAY initialization succeeded.\n\r");

        // init barrier_array
        init_user_sem_array();
        printk("> [INIT] user_sem_array initialization succeeded.\n\r");

        // init barrier_array
        init_user_mailbox_array();
        printk("> [INIT] user_mailbox_array initialization succeeded.\n\r");

        screen_clear();

        // 唤醒从核
        wakeup_other_hart();
    }
    else
    {
        setup_exception();
    }

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);
    // TODO:
    // Setup timer interrupt and enable all interrupt
    sbi_set_timer(get_ticks() + TIMER_INTERVAL); //设置时钟中断
    while (1)
    {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        enable_interrupt();
        __asm__ __volatile__("wfi\n\r" ::
                                 :);
        //do_scheduler();
    };
    return 0;
}
