#ifndef MM_H
#define MM_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
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

#include <type.h>
#include <pgtable.h>
static LIST_HEAD(freePageList);
#define MEM_SIZE 32
#define PAGE_SIZE 4096                // 4K
#define FREEHEAP 0xffffffc05d000000lu //堆
#define MAP_KERNEL 1
#define MAP_USER 2
#define LOAD 0
#define STORE 1
#define ALLOC_NO_AD 1
#define IN_SD 2

#define MAX_PAGE_NUM 3

#define INIT_KERNEL_STACK 0xffffffc050500000lu
#define INIT_KERNEL_STACK_MSTER 0xffffffc051000000lu
#define INIT_KERNEL_STACK_SLAVE 0xffffffc051001000lu
#define INIT_KERNEL_STACK 0xffffffc051000000lu
#define USER_STACK_ADDR 0xf00010000lu
#define FREEMEM (INIT_KERNEL_STACK + 2 * PAGE_SIZE)

/* Rounding; only works for n = power of two */
#define ROUND(a, n) (((((uint64_t)(a)) + (n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

extern ptr_t memCurr;
/* 记录回收的页的信息 */
typedef struct
{
    uintptr_t kva; //虚拟地址
    list_node_t list;
} free_page_t;

/* 记录进程持有页的信息 */
typedef struct
{
    uintptr_t vta;      //虚拟页面
    uintptr_t kva;      //物理页的虚拟地址
    PTE *phyc_page_pte; //指示虚拟页的PTE表项
    uintptr_t SD_place; //存储在磁盘的哪一个位置
    list_node_t list;
} page_t;

extern ptr_t allocPage();
extern void freePage(ptr_t baseAddr);
extern void *kmalloc(size_t size);
extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir);
extern uint32_t check_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode);
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);

#endif /* MM_H */
