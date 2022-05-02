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

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pcb->save_context = pt_regs;
    pt_regs->regs[2] = user_stack;
    pt_regs->regs[3] = (reg_t)__global_pointer$;
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;
    for (int i = 5; i < 32; i++)
    {
        pt_regs->regs[i] = 0;
    }
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;
    pcb->switch_context = st_regs;
    pcb->user_sp = user_stack;
    st_regs->regs[0] = pcb->type == USER_PROCESS || pcb->type == USER_THREAD ? (reg_t)&ret_from_exception : entry_point;
    st_regs->regs[1] = user_stack;
    for (int i = 2; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
}
#define TASK5
static void init_pcb()
{
    /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    int task3_total_number = num_timer_tasks + num_sched2_tasks;
    int task4_total_number = num_sched2_tasks + num_lock2_tasks;
    int task_total_number = num_timer_tasks + num_sched2_tasks + num_lock2_tasks;
    task_info_t *task;

#ifdef TASK3
    for (int i = 0; i < task3_total_number; i++)
    {
        if (i < num_timer_tasks)
            task = timer_tasks[i];
        else if (i < task4_total_number)
            task = sched2_tasks[i - num_timer_tasks];
#endif
#ifdef TASK4
        for (int i = 0; i < task4_total_number; i++)
        {
            if (i < num_sched2_tasks)
                task = sched2_tasks[i];
            else if (i < task4_total_number)
                task = lock2_tasks[i - num_sched2_tasks];
#endif
#ifdef TASKP2
            for (int i = 0; i < task_total_number; i++)
            {
                if (i < num_timer_tasks)
                    task = timer_tasks[i];
                else if (i < task3_total_number)
                    task = sched2_tasks[i - num_timer_tasks];
                else if (i < task_total_number)
                    task = lock2_tasks[i - task3_total_number];
#endif
#ifdef TASK5
                for (int i = 0; i < num_fork_tasks; i++)
                {
                    task = fork_tasks[i];
#endif

                    pcb[i].list.priority = task->priority;
                    pcb[i].prepriority = task->priority;
                    pcb[i].kernel_sp = allocPage(1);
                    pcb[i].user_sp = allocPage(1);

                    pcb[i].preempt_count = 0;

                    pcb[i].pid = process_id++;

                    pcb[i].type = task->type;

                    pcb[i].status = TASK_READY;

                    pcb[i].cursor_x = 0;
                    pcb[i].cursor_y = 0;
                    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i]);
                    list_add_priority(&pcb[i].list, &ready_queue);
                }
                /* remember to initialize `current_running`
     * TODO:
     */
                current_running = &pid0_pcb;
                printk("> [INIT] everything open.\n\r");
            }

            void syscall_exit(void)
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
                    syscall[i] = &syscall_exit;
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

                syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
                syscall[SYSCALL_GET_TICK] = &get_ticks;
                syscall[SYSCALL_GET_WALL_TIME] = &get_wall_time;
            }

            // jump from bootloader.
            // The beginning of everything >_< ~~~~~~~~~~~~~~
            int main()
            {
                // init Process Control Block (-_-!)
                init_pcb();
                printk("> [INIT] PCB initialization succeeded.\n\r");

                // read CPU frequency
                time_base = sbi_read_fdt(TIMEBASE);

                // init interrupt (^_^)
                init_exception();
                printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

                // init system call table (0_0)
                init_syscall();
                printk("> [INIT] System call initialized successfully.\n\r");

                // fdt_print(riscv_dtb);

                // init screen (QAQ)
                init_screen();
                printk("> [INIT] SCREEN initialization succeeded.\n\r");

                // init lock_array
                init_user_lock_array();
                printk("> [INIT] USER_LOCK_ARRAY initialization succeeded.\n\r");
                // TODO:
                // Setup timer interrupt and enable all interrupt
                reset_irq_timer();
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
