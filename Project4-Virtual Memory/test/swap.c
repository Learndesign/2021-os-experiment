#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>
#define MAX_ADDR 10
#define LOAD 1
#define STORE 2

int main()
{
    srand(clock());
    long addr_op[MAX_ADDR];
    long data[MAX_ADDR];
    /**
     * 0x10800000
     *
     *
     */
    for (int i = 0; i < MAX_ADDR; i++)
    {
        addr_op[i] = 0x10800000 + 0x2000 * i;
        *(long *)(addr_op[i]) = 1;
        data[i] = 1;
    }

    for (int i = 0; i < 20; i++)
    {
        long op = rand() % 3;
        long addr_t = rand() % 9;
        if (op == LOAD)
        {
            /* 取出数据 */
            sys_reflush();
            long data_t = *(long *)(addr_op[addr_t]);
            printf("> Load\t");
            printf(" the load addr:0x%lx, data: 0x%lx\t", addr_op[addr_t], data_t);
            if (data_t == data[addr_t])
            {
                printf(" Success! \n");
            }
            else
            {
                printf(" ERROR! \n");
            }
        }
        else
        {
            long data_t = rand();
            printf("> Store\t");
            printf(" the Store addr:0x%lx, data: 0x%lx\n", addr_op[addr_t], data_t);
            sys_reflush();
            *(long *)(addr_op[addr_t]) = data_t;
            data[addr_t] = data_t;
        }
    }
    return 0;
}