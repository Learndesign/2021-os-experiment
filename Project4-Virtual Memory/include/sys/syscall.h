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
// #include <mthread.h>
#include <stdint.h>
#include <os.h>

#define SCREEN_HEIGHT 80

void sys_show_exec();

extern long invoke_syscall(long, long, long, long, long);
extern int sys_fork(int);
void sys_sleep(uint32_t);
void sys_yield();
#define BINSEM_OP_LOCK 0   // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

int binsemget(int key);
int binsemop(int binsem_id, int op);

int sys_lock_init(int *);
void sys_lock_op(int, int);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();
void sys_delete();
int sys_get_char();
void sys_put_char(char ch);

long sys_get_tick();

void sys_exit(void);
int sys_kill(char *name);
int sys_waitpid(pid_t pid);
void sys_process_show(void);
void sys_screen_clear(void);
void sys_delete(void);
pid_t sys_getpid();

void sys_ps();
void sys_ls();
pid_t sys_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode);

void sys_semphore_init(int *, int *, int);
void sys_semphore_up(int, int *);
void sys_semphore_down(int, int *);
void sys_semphore_destory(int *, int *);

void sys_barrier_init(int *, int *, int *, unsigned int);
void sys_barrier_wait(int, int *, int);
void sys_barrier_destory(int *, int *, int *);

int sys_mailbox_open(char *);
void sys_mailbox_close(int);
int sys_mailbox_send(int, void *, int);
int sys_mailbox_recv(int, void *, int);
int sys_mailbox_send_recv(int *, void *, void *, int *);

pid_t sys_create_thread(uintptr_t entry_point, void *arg);
int thread_join(int thread);
#endif
