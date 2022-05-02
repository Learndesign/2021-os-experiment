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
#include <type.h>
#include <stdint.h>
#include <os.h>
 
#define SCREEN_HEIGHT 80
 
pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
pid_t sys_create_thread(uintptr_t entry_point, void* arg);
void sys_show_exec();

extern long invoke_syscall(long, long, long, long, long);
// extern int sys_fork(int); 
void sys_sleep(uint32_t);  
void sys_yield();
#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

int binsemget(int key);
int binsemop(int binsem_id, int op);

int sys_lock_init(int *);
void sys_lock_op(int, int);
void sys_priori(int,int);
int sys_fork();

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

int sys_get_id();
int sys_get_priorit();
long sys_get_tick();

void sys_exit(void);
int sys_kill(char * name);
int sys_waitpid(pid_t pid);
void sys_process_show(void);
void sys_screen_clear(void);
pid_t sys_getpid();
int sys_get_char();
void sys_put_char(char ch);
void sys_ps();
int sys_taskset(int, char *, int);
/*信号量*/
void sys_semphore_init(int *, int *, int );
void sys_semphore_up(int , int *);
void sys_semphore_down(int , int *);
void sys_semphore_destory(int *, int *);

/*屏障*/
void sys_barrier_init(int *, int *, int *, unsigned int);
void sys_barrier_wait(int , int *, int );
void sys_barrier_destory(int *, int *, int *);

/*信箱*/
void sys_mailbox_open(int *, char *);
void sys_mailbox_close(int );
int sys_mailbox_send(int , void *, int);
int sys_mailbox_recv(int , void *, int);
/*结合*/
int sys_mailbox_send_recv(int*, void *, void *, int *);

/* 网卡驱动 */
long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength);
void sys_net_send(uintptr_t addr, size_t length);
void sys_net_irq_mode(int mode);

/* 线程 */
int thread_join(int thread);

/* 文件系统 */
void sys_mkfs();
void sys_statfs();
void sys_cd(char *dirname);
void sys_mkdir(char *dirname);
void sys_rmdir(char *dirname);
void sys_ls(int mode, char * path);
void sys_touch(char *filename);
void sys_cat(char *filename);
int sys_fopen(char *name, int access);
int sys_fread(int fd, char *buff, int size);
int sys_fwrite(int fd, char *buff, int size);
void sys_close(int fd);
void sys_ln(char *source, char *link_name);
void sys_delete(void);
int sys_lseek(int fd, int offset, int whence);
int sys_rm(char *dirname);
#endif
