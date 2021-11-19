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

#include <os/list.h>

#define MAX_MAILBOX_NUM 16
#define MAX_MSG_LENGTH 64

typedef enum
{
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef enum
{
    USED,
    UNSED,
} user_lock_status_t;

typedef struct spin_lock
{
    volatile lock_status_t status;
} spin_lock_t;

typedef struct user_spin_lock
{
    volatile user_lock_status_t status;
} user_spin_lock_t;

typedef struct mutex_lock
{
    spin_lock_t lock;
    list_head block_queue;
} mutex_lock_t;

typedef struct user_lock
{
    user_spin_lock_t lock;
    mutex_lock_t kernel_lock;
    int id;
} user_lock_t;

typedef enum
{
    MBOX_OPEN,
    MBOX_CLOSE,
} mailbx_ststus_t;

typedef struct user_mailbox
{
    char *name;               //信箱名称
    char msg[MAX_MSG_LENGTH]; //信箱的内容
    int head;                 //信箱中放信的指针
    int tail;                 //信箱中收信的指针
    int msg_num;              //信箱中的字符数
    int visited;              //信箱正在被使用的数量
    mailbx_ststus_t status;   //信箱的状态
    mutex_lock_t *mbox_lock;  //保证信箱互斥访问的锁
    list_node_t empty;        //信箱为空时需要将接收进程阻塞
    list_node_t full;         //信箱为满时需要将发送进程阻塞
} user_mailbox_t;

/* init lock */
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

void do_mutex_lock_init(mutex_lock_t *lock);
void do_mutex_lock_acquire(mutex_lock_t *lock);
void do_mutex_lock_release(mutex_lock_t *lock);
void init_user_lock_array();
int lock_create();
void lock_jion(int lock_id, int op);
#define NUM_MAX_USER 16
#define USER_OP_LOCK 0
#define USER_OP_UNLOCK 1

user_mailbox_t user_mailbox_array[MAX_MAILBOX_NUM];
void init_user_mailbox_array();
int do_mbox_open(char *name);
void do_mbox_close(int mailbox_id);
int do_mbox_send(int mailbox_id, void *msg, int msg_length);
int do_mbox_recv(int mailbox_id, void *msg, int msg_length);
int do_mbox_send_recv(int *mailbox_id, void *send_msg, void *recv_msg, int *length);
#endif
