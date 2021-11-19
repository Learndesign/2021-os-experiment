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
#define SYSCALL_SLEEP 2

#define SYSCALL_LOCKCREATE 3
#define SYSCALL_LOCKJION 4

#define SYSCALL_YIELD 7

#define SYSCALL_FORK 8
#define SYSCALL_PRIORIT 9
#define SYSCALL_GET_ID 10
#define SYSCALL_GET_PRIORIT 11

//P3task1
#define SYSCALL_SCREEN_CLEAR 12
#define SYSCALL_SPAWN 13
#define SYSCALL_KILL 14
#define SYSCALL_PS 15
#define SYSCALL_WAITPID 16
#define SYSCALL_EXIT 17

#define SYSCALL_WRITE 20
#define SYSCALL_READ 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_DELETE 24

//get time
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_GET_WALL_TIME 32

//P3task2
#define SYSCALL_BARRIER_WAIT 33
#define SYSCALL_BARRIER_INIT 34
#define SYSCALL_SEM_INIT 35
#define SYSCALL_SEM_DESTORY 36
#define SYSCALL_SEM_UP 37
#define SYSCALL_SEM_DOWN 38

//P3 mailbox
#define SYSCALL_MBOX_OPEN 40
#define SYSCALL_MBOX_CLOSE 41
#define SYSCALL_MBOX_SEND 42
#define SYACALL_MBOX_RECV 43
#define SYSCALL_MBOX_SEND_RECV 44

#define SYSCALL_TASKSET 45

#endif
