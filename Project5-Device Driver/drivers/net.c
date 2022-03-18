#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];
//P5 TASK 4 two recv
int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t *frLength)
{
    // TODO:
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    /* pre addr */
    uintptr_t pre_addr = addr;
    uint32_t total_num = num_packet;
    /* total recv times */
    uint32_t recv_time = total_num / RXBD_CNT + ((total_num % RXBD_CNT) != 0 ? 1 : 0);
    /* kernel addr of addr */
    uintptr_t kaddr = get_kva_of(addr, current_running->pgdir);
    /* kernel length addr */
    uintptr_t krx_len = get_kva_of((uintptr_t)frLength, current_running->pgdir);
    /* enbale INT */
    // if(net_poll_mode == NIK_INT)
    //     INTENABLE(&EmacPsInstance);
    while (recv_time)
    {
        /* recv once time */
        uint32_t recv_num = recv_time == 1 ? total_num : RXBD_CNT;
        /* recv */
        EmacPsRecv(&EmacPsInstance, rx_buffers, recv_num);
        /* wait recv */
        EmacPsWaitRecv(&EmacPsInstance, recv_num, rx_len);
        for (int i = 0; i < recv_num; i++)
        {
            /* copy for user */
            kmemcpy((uint8_t *)kaddr, (uint8_t *)rx_buffers[i], rx_len[i]);
            /* length */
            *(size_t *)krx_len = rx_len[i];
            krx_len += sizeof(size_t);
            kaddr += rx_len[i];
            addr += rx_len[i];
        }
        recv_time--;
        total_num -= RXBD_CNT;
    }
    /* disable INT */
    // if(net_poll_mode == NIK_INT)
    //     INTDISABLE(&EmacPsInstance);
    return addr - pre_addr;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    // send all packet
    // maybe you need to call drivers' send function multiple times ?
    kmemcpy(&tx_buffer, addr, length);
    // send packet
    /* enbale INT */
    // if(net_poll_mode == NIK_INT)
    //     INTENABLE(&EmacPsInstance);
    /* send */
    EmacPsSend(&EmacPsInstance, &tx_buffer, length);
    /* wait */
    EmacPsWaitSend(&EmacPsInstance);
    /* disable INT */
    // if(net_poll_mode == NIK_INT)
    //     INTDISABLE(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    // TODO:
    // turn on/off network driver's interrupt mode
    /**
     * the mode = 0 will be the poll pattern
     * the mode = 1 will be the time break
     * the mode = 2 will be the nic interrupt
     */
    net_poll_mode = mode;
    //网卡中断打开
    if (net_poll_mode == NIK_INT)
        INTENABLE(&EmacPsInstance);
    else
        INTDISABLE(&EmacPsInstance);
}
