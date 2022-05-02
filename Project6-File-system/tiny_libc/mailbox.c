#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>

mailbox_t mbox_open(char *name)
{
    // TODO:
    mailbox_t mbox_id ;
    sys_mailbox_open(&mbox_id,name);
    return mbox_id;
}

void mbox_close(mailbox_t mailbox)
{
    // TODO:
    sys_mailbox_close((int)mailbox);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return sys_mailbox_send(mailbox, msg, msg_length);
}

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return sys_mailbox_recv(mailbox, msg, msg_length);
}
