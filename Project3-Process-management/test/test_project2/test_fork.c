#include <test2.h>
#include <mthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/syscall.h>

void fork_task1(void)
{
    int i;
    char child_priority;
    int print_location = 3;
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        int id = sys_getpid();
        int priorit = sys_get_priorit();
        if (id == 1)
        {
            printf("This's father_%d process(%d),  priority: %d\n", id, i, priorit);
            sys_move_cursor(1, print_location + 10);
            printf("fork one child process: input the priority of chlid:\n");
            child_priority = sys_read();
            if (child_priority <= '9' && child_priority >= '0')
            {
                int priority = child_priority - '0';
                int pcb_id = sys_fork(SYSCALL_FORK);
                id = sys_get_id();
                if (id == 1)
                    sys_priori(priority, pcb_id);
            }
        }
        else
        {
            sys_move_cursor(1, print_location + id - 1);
            printf("This's child_%d process(%d),  priority: %d\n", id - 1, i, priorit);
        }
    }
}