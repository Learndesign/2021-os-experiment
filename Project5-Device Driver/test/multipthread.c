#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mailbox.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#define SHMP_KEY 42
#define LOCK_BINSEM_KEY 42
#define OP_NUM 50000
#define UNLOCK 0
#define LOCK 1


typedef struct
{
    uint32_t status;
} user_spin_lock_t;

user_spin_lock_t user_lock;

void smp_init()
{
    user_lock.status = UNLOCKED;
}

void lock()
{
    while(atomic_exchange(&user_lock.status, LOCKED));
}

void unlock()
{
    user_lock.status = UNLOCKED;
}
/*偶数*/
void op_odd(void *arg){
    long * op_num = (long *)arg;
    for(int i = 1; i <= OP_NUM; i++){ 
        // sys_move_cursor(1,10);
        // printf("%d", 2 * i);
        lock();
        *op_num += (2 * i);
        unlock();
    }  
    /* 显示的退出，否则将会报出指令缺页，由于其没有链接crt0.S */     
    sys_exit();        
}
/* 奇数 */
void op_even(void *arg){
    long * op_num = (long *)arg;
    for(int i = 0; i < OP_NUM; i++){ 
        // sys_move_cursor(1,9);
        // printf("%d", 2 * i + 1);
        lock();
        *op_num += (2 * i + 1);
        unlock();
    }     
}

int main(int argc, char* argv[])
{
    smp_init();
    /* share mem */
    long * op_num = (long *)shmpageget(SHMP_KEY);

    mthread_t odd;
    /* creat one pthread */
    mthread_create(&odd, op_odd, (void*)op_num);
    /* main pthread */
    op_even(op_num);
    /* wait */
    mthread_join(odd);
    /* delete the share mem */  
    shmpagedt((void*)op_num);
    return 0;  
}