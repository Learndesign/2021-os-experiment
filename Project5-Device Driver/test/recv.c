#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <os.h>

#define PRINTF_NUM 6
#define MAX_RECV_CNT 100
char recv_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
size_t recv_length[MAX_RECV_CNT];

int main(int argc, char *argv[])
{
    // printf("%lx \n\r", (uintptr_t) argv);
    // printf("%lx \n\r", (uintptr_t) argv[0]);
    // printf("%lx \n\r", (uintptr_t) argv[1]);
    // printf("%lx \n\r", (uintptr_t) argv[2]);
    //uintptr_t baseAddr = ((uintptr_t) argv) >> 12 << 12;
    //char* tmpAddr = (char*) baseAddr;
    /*for (int i = 100; i < 128; ++i) {
        printf("0x%lx : ", &tmpAddr[i*32]);
        for (int j = 0; j < 32; ++j) {
            printf("%02x ", (uint32_t) tmpAddr[i*32 + j]);
        }
        printf("\n");
    }
    printf("\n");*/
    // for(;;);
    int mode = 0;
    int size = 1;
    if (argc > 1)
    {
        size = atol(argv[1]);
        printf("%d \n", size);
    }
    if (argc > 2)
    {
        // if (strcmp(argv[2], "1") == 0) {
        //     mode = 1;
        // }
        mode = atol(argv[2]);
        if (mode != 0 && mode != 1 && mode != 2)
        {
            printf("> [Error] enter the 0, 1, 2\n");
            return 0;
        }
        // printf("%d \n", mode);
    }

    sys_net_irq_mode(mode);

    sys_move_cursor(1, 1);
    printf("[RECV TASK] start recv(%d):                    ", size);

    int ret = sys_net_recv(recv_buffer, size * sizeof(EthernetFrame), size, recv_length);
    printf("%d\n", ret);
    char *curr = recv_buffer;
    for (int i = 0; i < size; ++i)
    {
#if defined PRINTF_NUM
        if (i >= size - PRINTF_NUM)
            printf("packet %d:\n", i);
        // printf("packet %d:                                          %d\n", i, recv_length[i]);
        for (int j = 0; j < (recv_length[i] + 15) / 16; ++j)
        {
            for (int k = 0; k < 16 && (j * 16 + k < recv_length[i]); ++k)
            {
                if (i >= size - PRINTF_NUM)
                    printf("%02x ", (uint32_t)(*(uint8_t *)curr));
                ++curr;
            }
            if (i >= size - PRINTF_NUM)
                printf("\n");
        }
#endif // PRINTF_NUM
#if defined ALL
        printf("packet %d:                                          %d\n", i, recv_length[i]);
        for (int j = 0; j < (recv_length[i] + 15) / 16; ++j)
        {
            for (int k = 0; k < 16 && (j * 16 + k < recv_length[i]); ++k)
            {
                printf("%02x ", (uint32_t)(*(uint8_t *)curr));
                ++curr;
            }
            printf("\n");
        }
#endif // ALL
    }

    return 0;
}