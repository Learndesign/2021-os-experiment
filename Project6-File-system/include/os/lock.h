/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
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

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_
#define MAX_LOCK_NUM 16
#define MAX_WAIT_QUEUE 16
#define MAX_BARRIER_NUM 16
#define MAX_MAILBOX_NUM 16
#define MAX_MSG_LENGTH 64

#include <os/list.h>
/*零代表上锁操作，1代表解锁操作*/
#define USER_LOCK 0
#define USER_NULOCK 1

/*锁的状态*/
typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

/*锁是否被使用*/
typedef enum {
    USED,
    UNSED,
} user_lock_status_t;

typedef enum {
    MBOX_OPEN,
    MBOX_CLOSE,
} mailbx_ststus_t;
/*自旋锁*/

/*自旋锁*/
typedef struct spin_lock
{
    volatile lock_status_t status;
} spin_lock_t;

/*内核直接操作的锁结构*/
typedef struct mutex_lock
{
    spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

//内核当中保存的可以供用户使用的锁的结构
typedef struct kernel_lock
{
    int status;
    int id;
    int key;//关键字
    mutex_lock_t kernel_lock;
}kernel_lock_t;

/*内核操作的信号量*/
typedef struct kernel_semwait
{
    int status;//该队列是否被使用过
    int wt_num;
    int wt_now;
    list_node_t wait_queue;
}kernel_semwait_t;

typedef struct kernel_barrier
{
    int status;//该队列是否被使用过
    list_node_t wait_queue;
}kernel_barrier_t;

typedef struct kernel_mailbox
{
    char * name;                     //信箱名称
    char msg[MAX_MSG_LENGTH];        //信箱的内容
    int head,tail;                   //写信箱的下标
    int msg_num;                     //信箱中的字符数
    int visited;                     //信箱正在被使用的数量
    mailbx_ststus_t status;          //信箱的状态
    mutex_lock_t * mbox_mutex;       //同样只保留了信箱id
    list_node_t empty;               //当空时需要将recv阻塞在此处
    list_node_t full;                //当满时需要将send阻塞在此处
}kernel_mailbox_t;



kernel_lock_t User_Lock[MAX_LOCK_NUM];
kernel_semwait_t User_Sem_Wait[MAX_WAIT_QUEUE];
kernel_barrier_t User_Barrier_Wait[MAX_BARRIER_NUM];
kernel_mailbox_t User_Mailbox[MAX_MAILBOX_NUM];
/*the LOCK that show id to the users*/
/* init lock */
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

/*内核的锁函数*/
void do_mutex_lock_init(mutex_lock_t *lock);
void do_mutex_lock_acquire(mutex_lock_t *lock);
void do_mutex_lock_release(mutex_lock_t *lock);
void do_mutex_lock_release_all(mutex_lock_t *lock, int mutex_id);

/*给用户的锁结构的接口*/
void init_lock_array();
int do_mutex_init(int * mutex_id, int key);
void do_mutex_op(int mutex_id, int op);

/*信号量原语*/
void do_semphore_init(int * id, int * value, int val);
void do_semphore_up(int mutex_id,int * semphore);
void do_semphore_down(int mutex_id,int * semphore);
void do_semphore_destory(int * mutex_id, int * semphore);

/*屏障原语*/
void do_barrier_init(int * barrier_id, int * wt_now, int * wt_num, unsigned count);
void do_barrier_wait(int  barrier_id, int * wt_now, int wt_num);
void do_barrier_destory(int * barrier_id, int * wt_now, int * wt_num);

/*信箱通信*/
void do_mbox_open(int *mailbox_id, char * name);
void do_mbox_close(int mailbox_id);
int do_mbox_send(int mailbox_id, void *msg, int msg_length);
int do_mbox_recv(int mailbox_id, void *msg, int msg_length);
int do_mbox_send_recv(int *mailbox_id, void *send_msg, void * recv_msg, int * length);

#endif
