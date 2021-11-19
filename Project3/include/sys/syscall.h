/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
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

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
#include <stdint.h>
#include <os.h>

extern long invoke_syscall(long, long, long, long, long);

void sys_sleep(uint32_t);

int sys_read();
void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();
void sys_delete();

int sys_getpid();
int sys_get_priorit();

int sys_lock_init();
void sys_lock_jion(int lock_id, int op);

long sys_get_timebase();
long sys_get_tick();

int sys_get_id();
int sys_get_priorit();
void sys_priorit(int priorit, int pcb_id);

uint32_t sys_get_wall_time(uint32_t *time_elapsed);

void sys_clear();
void sys_ps();
void sys_exit();
int sys_waitpid(pid_t pid);
int sys_kill(pid_t pid);
pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode);

void sys_barrier_wait(int id, int count);
int sys_barrier_init();
int sys_sem_init(int val);
void sys_sem_destory(int id);
void sys_sem_up(int id);
void sys_sem_down(int id);

int sys_mailbox_open(char *);
void sys_mailbox_close(int);
int sys_mailbox_send(int, void *, int);
int sys_mailbox_recv(int, void *, int);

pid_t sys_getpid();
int sys_get_char();

int sys_mailbox_send_recv(int *, void *, void *, int *);
int sys_taskset(task_info_t *, int, int, int);

#endif
