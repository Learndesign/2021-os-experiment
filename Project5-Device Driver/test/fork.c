#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>
#define MAX_ADDR 5
long addr[] = {
    0x10800000,
    0x630500000,
    0x40134000,
    0x962300000,
    0xa80200000
    // 0xa0000320,
    // 0xb0300420
};

int main(){   
    int father = sys_getpid();
    for(int i = 0; i < MAX_ADDR; i++){
        *(long *)addr[i] = 1;
    }
    // for (int i = 0; i < MAX_ADDR; i++){
    //     long data = *(long *)addr[i];
    //     printf("the addr:0x%lx data:0x%lx\n",addr[i], data);
    // }
    srand(clock());
    // sys_fork();
    int father_pid = sys_getpid();
    int child_pid = sys_fork();
    // int offset = child_pid - father_pid;
    if(sys_getpid() == father_pid)
        sys_move_cursor(1, 1);
    else
        sys_move_cursor(1, 2);
    printf("My pid is %d, I fork one child pid: %d\n", sys_getpid(), child_pid);
    // sys_move_cursor(1, sys_getpid()+2);
    for (int i = 0; i < MAX_ADDR; i++){
        if(sys_getpid() == father_pid)
            sys_move_cursor(1, 3+i);
        else
            sys_move_cursor(1, 3 + MAX_ADDR + 1 +i);
        if(sys_getpid() == father_pid){
            long write_data = rand();
            *(long *)addr[i] = write_data;
            printf("I am the father(%d) process, I write 0x%lx to the addr: 0x%lx\n", sys_getpid(), write_data, addr[i]);
            // sys_reflush();
        }else{
            long write_data = rand();
            *(long *)addr[i] = write_data;
            printf("I am the child(%d) process, I write 0x%lx to the addr: 0x%lx\n", sys_getpid(), write_data, addr[i]);
            // sys_reflush();
        }
    }
    sys_sleep(20);
    for (int i = 0; i < MAX_ADDR; i++){
        if(sys_getpid() == father_pid)
            sys_move_cursor(1, 3 + 2 * MAX_ADDR + 2 + i);
        else
            sys_move_cursor(1, 3 + 3 * MAX_ADDR + 3 + i);
        long read_data = *(long *)addr[i];
        if(sys_getpid() == father_pid){
            printf("I am the father(%d) process, I read 0x%lx from the addr: 0x%lx", sys_getpid(), read_data, addr[i]);
            // sys_reflush();
        }else{
            printf("I am the child(%d) process, I read 0x%lx from the addr: 0x%lx", sys_getpid(), read_data, addr[i]);
            // sys_reflush();
        }         
    }
    sys_sleep(20);
    for(int i=0 ; ;i++){
        if(sys_getpid() == father_pid)
            sys_move_cursor(1, 35);
        else
            sys_move_cursor(1, 36);
        printf("I am the %d process, I will print %d", sys_getpid(), i);
        // if(id != father)
        //     sys_sleep(1);
        if(sys_getpid() != father_pid && i%100 == 0){
            sys_sleep(3);
        }
    }

    return 0;
}