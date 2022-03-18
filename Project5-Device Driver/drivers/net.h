#ifndef NET_H
#define NET_H

#include <type.h>
#define POLL_PATTERN 0
#define TIME_BREAK 1
#define NIK_INT 2

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength);
void do_net_send(uintptr_t addr, size_t length);
void do_net_irq_mode(int mode);

extern int net_poll_mode;

#endif //NET_H
