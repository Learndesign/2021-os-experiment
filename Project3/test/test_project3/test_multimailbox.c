#include <time.h>
#include <test_project3/test3.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>
#include <string.h>

void testmultimailbox(int mailbox_id)
{
    /*open three mialbox*/
    char recv_msg[MAX_MBOX_LENGTH] = {0};
    char send_msg[MAX_MBOX_LENGTH] = {0};

    int send_bytes = 0; //进程发送总字节数
    int recv_bytes = 0; //进程接收总字节数
    int pid = sys_getpid();

    mailbox_t mialbox_1 = mbox_open("mialbox_1");
    mailbox_t mialbox_2 = mbox_open("mialbox_2");
    mailbox_t mialbox_3 = mbox_open("mialbox_3");
    sys_sleep(1);
    for (;;)
    {

        int length[2];
        int rand_mailbox[2];
        length[0] = (rand() % MAX_MBOX_LENGTH) / 2;
        length[1] = (rand() % MAX_MBOX_LENGTH) / 2;

        //接收邮箱ID
        rand_mailbox[1] = rand() % 3;

        //发送邮箱ID
        while ((rand_mailbox[0] = rand() % 3) == mailbox_id)
            ;

        generateRandomString(send_msg, length[0]);

        int result = sys_mailbox_send_recv(rand_mailbox, send_msg, recv_msg, length);

        sys_move_cursor(1, 9 * mailbox_id + 1);
        if (result == 1)
        {
            printf("process[%d] succeed to recv %d bytes from mailbox[%d]\n", pid, length[1], rand_mailbox[1]);
            printf("The msg:: %s\n", recv_msg);
            printf("\n");
            printf("\n");
            recv_bytes += length[1];
        }
        else if (result == 2)
        {
            printf("process[%d] succeed to send %d  bytes to mailbox[%d]\n", pid, length[0], rand_mailbox[0]);
            printf("The msg: %s\n", send_msg);
            printf("\n");
            printf("\n");
            send_bytes += length[0];
        }
        else if (result == 3)
        {
            printf("process[%d] succeed to recv %d bytes from mailbox[%d]\n", pid, length[1], rand_mailbox[1]);
            printf("The msg: %s\n", recv_msg);
            printf("process[%d] succeed to send %d bytes to mailbox[%d]\n", pid, length[0], rand_mailbox[0]);
            printf("The msg: %s\n", send_msg);
            recv_bytes += length[1];
            send_bytes += length[0];
        }
        printf("process[%d] has send total %d bytes\n", pid, send_bytes);
        printf("process[%d] has recv total %d bytes\n", pid, recv_bytes);
        sys_sleep(1);
    }
}

void test_multibox(void)
{
    struct task_info task = {(uintptr_t)&testmultimailbox,
                             USER_PROCESS};
    for (int i = 0; i < 3; i++)
    {
        sys_spawn(&task, i, ENTER_ZOMBIE_ON_EXIT);
    }

    sys_exit();
}
