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

#ifndef OS_SYSCALL_NUMBER_H_
#define OS_SYSCALL_NUMBER_H_

#define IGNORE 0
#define NUM_SYSCALLS 64

/* define */
#define SYSCALL_EXEC 0 //p3
#define SYSCALL_EXIT 1 //p3
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3    //p3
#define SYSCALL_WAITPID 4 //p3
#define SYSCALL_PS 5      //p3
#define SYSCALL_GETPID 6  //p3
#define SYSCALL_TASKSET 7 //p3
#define SYSCALL_LS 8      //p4

#define SYSCALL_YIELD 10

#define SYSCALL_LOCK_INIT 13
#define SYSCALL_LOCK_OP 14

#define SYSCALL_WRITE 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_DELETE 24

#define SYSCALL_SCREEN_CLEAR 27 //p3

#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_GET_CHAR 32 //p3
#define SYSCALL_PUT_CHAR 33 //p3

/*信号量*/
#define SYSCALL_SEM_INIT 37
#define SYSCALL_SEM_UP 38
#define SYSCALL_SEM_DOWN 39
#define SYSCALL_SEM_DESTORY 40

/*屏障*/
#define SYSCALL_BARRIER_INIT 43
#define SYSCALL_BARRIER_WAIT 44
#define SYSCALL_BARRIER_DESTORY 45

/*信箱*/
#define SYSCALL_MBOX_OPEN 50
#define SYSCALL_MBOX_CLOSE 51
#define SYSCALL_MBOX_SEND 52
#define SYACALL_MBOX_RECV 53
#define SYSCALL_MBOX_SEND_RECV 54

/*线程*/
#define SYSCALL_THREAD_CREATE 56
#define SYSCALL_THREAD_JION 57
#endif
