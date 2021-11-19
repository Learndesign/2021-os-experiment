/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/time.h>
#include <os/lock.h>

#define NUM_MAX_TASK 16
extern void ret_from_exception();

typedef enum
{
    B_USED,
    B_UNSED,
} user_barrier_status_t;

typedef struct sem
{
    int id;
    int current_num;
    user_barrier_status_t status;
    list_head block_queue;
} user_sem_t;

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    reg_t regs[32];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;

typedef enum
{
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
} task_status_t;

typedef enum
{
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum
{
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;
/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    /* previous, next pointer */
    list_node_t list;

    /* process id */
    pid_t pid;

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING */
    task_status_t status;

    /*origin define priority*/
    priority_t prepriority;

    /*base address of context*/
    regs_context_t *save_context;
    switchto_context_t *switch_context;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /* timer for sleep */
    timer_t timer;

    /*exit mode*/
    spawn_mode_t mode;

    /* parent process */
    pid_t parent;

    /* parent process */
    pid_t child;

    /* handlelock */
    int lock_num;
    mutex_lock_t *locks[NUM_MAX_TASK];

    /**/
    uint32_t mask;

    /* name  */
    // char name[16];

    /*~zero  means killed */
    // int killed;

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
    priority_t priority;
} task_info_t;

/* ready queue to run */
extern list_head ready_queue;
extern list_head wait_queue;
/* current running task PCB */
extern pcb_t *volatile current_running;
extern pcb_t *volatile current_running_master;
extern pcb_t *volatile current_running_slave;
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern uint64_t kernel_stack[NUM_MAX_TASK];
extern uint64_t user_stack[NUM_MAX_TASK];
extern pcb_t pid0_pcb_master;
extern const ptr_t pid0_stack_m;
extern pcb_t pid0_pcb_slave;
extern const ptr_t pid0_stack_s;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);
void do_priori(int priori, int pcb_id);
int do_get_id();
int do_get_priorit();
int do_fork();

extern pid_t do_spawn(task_info_t *task, void *arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();

user_sem_t user_sem_array[NUM_MAX_USER];
extern void init_user_barrier_array();
extern int user_barrier_create();
extern int do_barrier_wait(int id, int count);

extern int user_sem_create(int value);
extern void user_semaphore_wait(int id);
extern void user_semaphore_signal(int id);
extern int user_sem_destory(int i);

#endif
