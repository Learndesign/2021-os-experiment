#include <sys/syscall.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdatomic.h>
#include <mthread.h>
#include <string.h>
#include <assert.h>
#define SHMP_KEY 42
#define LOCK_BINSEM_KEY 42
#define OP_NUM 50000
#define ODD 1//奇数
#define EVEN 2//偶数

int main(int argc, char* argv[])
{
    /* share mem */
    long * op_num = (long *)shmpageget(SHMP_KEY);
    int type;
    int binsem_id = binsemget(LOCK_BINSEM_KEY);
    /* choose the choose depend on the argv */
    if(!strcmp(argv[1], "odd"))
        type = ODD;
    else
        type = EVEN;
    /* 主进程计算偶数 */
    if(type == ODD){
        for(int i = 0; i < OP_NUM; i++){ 
            binsemop(binsem_id, BINSEM_OP_LOCK);
            *op_num += (2 * i + 1) ;
            binsemop(binsem_id, BINSEM_OP_UNLOCK);
        }        
    }else{
    /* 次进程计算奇数 */
        for(int i = 1; i <= OP_NUM; i++){ 
            binsemop(binsem_id, BINSEM_OP_LOCK);
            *op_num += (2 * i);
            binsemop(binsem_id, BINSEM_OP_UNLOCK);
        }                
    }
    /* delete the share mem */
    shmpagedt((void*)op_num);
    return 0;
}



