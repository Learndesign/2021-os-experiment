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

#include <context.h>
#include <pgtable.h>
#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <user_programs.h>
#include <os/elf.h>
#define NUM_MAX_TASK 16
// #define PRIORITY
#define NO_PRIORITY

extern void ret_from_exception();

typedef enum
{
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum
{
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

/*标示进程和线程的种类，用户与内核*/
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
    /*内核栈和用户栈*/
    reg_t kernel_sp; //内核栈sp
    reg_t user_sp;   //用户栈sp

    // count the number of disable_preempt(不能抢占)
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /*该结构体是否被使用过*/
    uint64_t used;

    char *name;
    /* 队列，可反推pcb，得到tp */
    list_node_t list;
    /*等待队列*/
    list_head wait_list;

    /*32个寄存器与上下文切换14个寄存器的起始地址*/
    regs_context_t *save_context;
    switchto_context_t *switch_context;
    /* 进程 id */
    pid_t pid;

    /* 任务类型 kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /*睡眠时间截止*/
    uint64_t end_time;

    /*所睡眠的内核id*/
    uint32_t sleep_core_id;

    /* mask used for taskset */
    uint32_t mask;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /*该进程所持有的锁*/
    uint32_t handle_mutex[MAX_LOCK_NUM];

    /* PGDIR */
    PTE pgdir;

    /* clock pointer */
    list_node_t *clock_point;

    /* page_table */
    list_head *pge_h;

    /* 页表数 */
    uint32_t pge_num;

    /* 如果是一个创建的线程 */
    uint32_t origin;

    /* 滚屏使用 */
    int cursor_y_base;

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point; //进程的入口地址
    task_type_t type;  //进程的种类
} task_info_t;

/* ready queue to run */
extern list_head ready_queue;

extern list_head net_recv_queue;
extern list_head net_send_queue;

/* current running task PCB */
extern pcb_t *volatile current_running;
/*双核操作*/
pcb_t *volatile current_running_master;
pcb_t *volatile current_running_slave;
// extern pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
// extern pcb_t kernel_pcb[NR_CPUS];
/*master*/
extern pcb_t pid0_pcb_master;
extern const ptr_t pid0_stack_m;
/*slave*/
extern pcb_t pid0_pcb_slave;
extern const ptr_t pid0_stack_s;

extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[]);
void cancel_kernel_map();
/* 回收物理页操作 */
void recycle(pcb_t *recy_pcb);
/* 关闭W位 */
void close_pte_W(pcb_t *cw_pcb);
/* 复制指针及其所有的信息 */
void copy_pge(list_head *des, list_head *scr);

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);
void do_priori(int, int);
int do_fork();
int do_get_id();
int do_get_priorit();
pcb_t *get_higher_process(list_head *, uint64_t);
uint32_t do_get_time_test(uint32_t *test_time_elapsed);

int find_name_process(char *name);
int find_pcb();
void do_block(list_node_t *, list_head *queue); //上锁
void do_unblock(list_node_t *);                 //解锁
void do_ps();

extern pid_t do_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern void kill_pcb(pcb_t *del_pcb);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern pid_t do_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode);
extern void do_ls();

list_node_t *get_process_for_core(int cpu_id);
int do_thread_create(void (*start_routine)(void *), void *arg);
int do_thread_join(int thread);
#endif
