#include <stdio.h>
#include <string.h>
#include <mthread.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <os.h>

#define RECV_NUM 2
char recv_buffer[RECV_NUM * sizeof(EthernetFrame)];
size_t recv_length[RECV_NUM];

void recv(void *arg){
    int pid;
    char *thread_buff;
    size_t *thread_length;
    for(int i = 0; ;i++){ 
        if((pid = sys_getpid()) % 2){
            sys_move_cursor(1, 12);            
            printf("My pid : %d, I will recv the %x\n", pid, 50001);
            thread_buff = (char *)&recv_buffer[0];
            thread_length = (size_t *)&recv_length[0];
        }else {
            sys_move_cursor(1, 1);
            printf("My pid : %d, I will recv the %x\n", pid, 58688);
            thread_buff = (char *)&recv_buffer[sizeof(EthernetFrame)];
            thread_length = (size_t *)&recv_length[1];        
        }
        /* recv one loop */
        int ret = sys_net_recv(thread_buff, sizeof(EthernetFrame), 1, thread_length);
        printf("%d\n", ret);
        char *curr = thread_buff;
        printf("packet %d:                                          %d\n", i, *thread_length);
        for (int j = 0; j < (*thread_length + 15) / 16; ++j) {
            for (int k = 0; k < 16 && (j * 16 + k < *thread_length); ++k) {
                printf("%02x ", (uint32_t)(*(uint8_t*)curr));
                ++curr;
            }
            printf("\n");
        } 
    }
    sys_exit();
}



int main(int argc, char *argv[])
{
    /* NTK_INT */
    sys_net_irq_mode(2);
    mthread_t id[RECV_NUM];
    int place[RECV_NUM];
    /* creat two thread */
    for (int i = 0; i < RECV_NUM; i++){
        place[i] = i;
        mthread_create(&id[i], &recv, &place[i]);
    }

    /* wait */
    for (int i = 0; i < RECV_NUM; i++){
        mthread_join(id[i]);
    }
    return 0;

}