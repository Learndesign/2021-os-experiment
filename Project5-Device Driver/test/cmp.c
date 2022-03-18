#include <sys/syscall.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <stdatomic.h>
#include <mthread.h>
#include <assert.h>
#define NUM_CPU 2
#define SHMP_KEY 42
#define LOCK_BINSEM_KEY 42

int main(int argc, char* argv[])
{
    
    int print_location = 1;
    if(argc > 1){
        print_location = atol(argv[0]);
    }
    sys_move_cursor(1, print_location);
    printf("> begin the multi process test:");
    pid_t pid[NUM_CPU];
    /* share mem */
    long * op_num = (long *)shmpageget(SHMP_KEY);
    char multiprocess[] = "multiprocess";
    char *odd_argv[] = {
        multiprocess,
        "odd"
    };
    char *even_argv[]={
        multiprocess,
        "even"
    };
    /* multi process tesst begin */
    uint64_t singleCoreBegin = clock();

    pid[0] = sys_exec(multiprocess, 2, odd_argv, AUTO_CLEANUP_ON_EXIT);
    pid[1] = sys_exec(multiprocess, 2, even_argv, AUTO_CLEANUP_ON_EXIT);
    
    for(int i = 0; i < NUM_CPU; i++){
        sys_waitpid(pid[i]);
    }
    uint64_t singleCoreEnd = clock();
    sys_move_cursor(1, print_location+1);
    printf("the multi process test has Done, the result: %ld, cost: %ld", 
            *op_num, singleCoreEnd - singleCoreBegin);
    sys_move_cursor(1, print_location+3);
    /* set 0 for the share mem */
    *op_num = 0;
    sys_sleep(3);

    printf("> begin the multi pthread test:");

    char *multipthread_argv[] ={
        "multipthread"
    };
    /* begin the multi pthread */
    uint64_t multiCoreBegin = clock();
    pid_t p_pid = sys_exec("multipthread", 1, multipthread_argv, AUTO_CLEANUP_ON_EXIT);
    sys_waitpid(p_pid);
    uint64_t multiCoreEnd = clock();

    sys_move_cursor(1, print_location+4);
    printf("the multi pthread test has Done, the result: %ld, cost: %ld", 
            *op_num, multiCoreEnd - multiCoreBegin);
    /* dt the share mem */
    shmpagedt((void*)op_num);
    sys_sleep(1);
    return 0;
}