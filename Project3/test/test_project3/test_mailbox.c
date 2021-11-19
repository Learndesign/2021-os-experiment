#include <time.h>
#include <test_project3/test3.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

// struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
// struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

static const char initReq[] = "clientInitReq";
static const int initReqLen = sizeof(initReq);
int serve_number = 0;  //number_of_serve
int client_number = 0; //number_of_client

struct MsgHeader
{
    int length;
    uint32_t checksum;
    pid_t sender;
};

const uint32_t MOD_ADLER = 65521;

/* adler32 algorithm implementation from: https://en.wikipedia.org/wiki/Adler-32 */
uint32_t adler32(unsigned char *data, size_t len)
{
    uint32_t a = 1, b = 0;
    size_t index;

    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

int clientInitReq(const char *buf, int length)
{
    if (length != initReqLen)
        return 0;
    for (int i = 0; i < initReqLen; ++i)
    {
        if (buf[i] != initReq[i])
            return 0;
    }
    return 1;
}
/*接收进程*/
void strServer(void)
{
    char msgBuffer[MAX_MBOX_LENGTH];
    struct MsgHeader header;
    int64_t correctRecvBytes = 0;
    int64_t errorRecvBytes = 0;
    int64_t blockedCount = 0;
    int clientPos = 1;
    int serve_id = ++serve_number;
    mailbox_t mq = mbox_open("str-message-queue");
    mailbox_t posmq = mbox_open("pos-message-queue");
    sys_move_cursor(1, serve_number);
    printf("[Server %d]: server started\n", serve_id);

    sys_sleep(1);

    for (;;)
    {
        blockedCount += mbox_recv(mq, &header, sizeof(struct MsgHeader));
        blockedCount += mbox_recv(mq, msgBuffer, header.length);

        uint32_t checksum = adler32(msgBuffer, header.length);
        if (checksum == header.checksum)
        {
            correctRecvBytes += header.length;
        }
        else
        {
            errorRecvBytes += header.length;
        }

        sys_move_cursor(1, serve_id + client_number);
        printf("[Server %d]: recved msg from %d (blocked: %ld, correctBytes: %ld, errorBytes: %ld)\n",
               serve_id, header.sender, blockedCount, correctRecvBytes, errorRecvBytes);

        if (clientInitReq(msgBuffer, header.length))
        {
            mbox_send(posmq, &clientPos, sizeof(int));
            ++clientPos;
        }

        sys_sleep(1);
    }
}

int clientSendMsg(mailbox_t /***/ mq, const char *content, int length)
{
    int i;
    char msgBuffer[MAX_MBOX_LENGTH] = {0};
    struct MsgHeader *header = (struct MsgHeader *)msgBuffer;
    char *_content = msgBuffer + sizeof(struct MsgHeader);
    header->length = length;
    header->checksum = adler32(content, length);
    header->sender = sys_getpid();

    for (i = 0; i < length; ++i)
    {
        _content[i] = content[i];
    }
    return mbox_send(mq, msgBuffer, length + sizeof(struct MsgHeader));
}

void generateRandomString(char *buf, int len)
{
    static const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-={};|[],./<>?!@#$%^&*";
    static const int alphaSz = sizeof(alpha) / sizeof(char);
    int i = len - 2;
    buf[len - 1] = '\0';
    while (i >= 0)
    {
        buf[i] = alpha[rand() % alphaSz];
        --i;
    }
}
/*发送进程*/
void strGenerator(void)
{
    client_number++;
    mailbox_t mq = mbox_open("str-message-queue");
    mailbox_t posmq = mbox_open("pos-message-queue");

    int len = 0;
    /*change the int to char*/
    int strBuffer[MAX_MBOX_LENGTH - sizeof(struct MsgHeader)];
    clientSendMsg(mq, initReq, initReqLen);
    int position = 0;

    mbox_recv(posmq, &position, sizeof(int));

    int blocked = 0;
    int64_t bytes = 0;

    sys_move_cursor(1, position);
    printf("[Client %d]: server started\n", position);

    sys_sleep(1);

    for (;;)
    {
        len = (rand() % ((MAX_MBOX_LENGTH - sizeof(struct MsgHeader)) / 2)) + 1;

        generateRandomString(strBuffer, len);

        blocked += clientSendMsg(mq, strBuffer, len);

        bytes += len;

        sys_move_cursor(1, position);
        printf("[Client %d]: send bytes: %ld, blocked: %d\n", position,
               bytes, blocked);
        sys_sleep(1);
    }
}
