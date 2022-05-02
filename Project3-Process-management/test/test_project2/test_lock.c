#include <test2.h>
#include <mthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <os/lock.h>
static int is_init = FALSE;
static char blank[] = {"                                             "};
/* if you want to use mutex lock, you need define MUTEX_LOCK */
//static mthread_mutex_t mutex_lock;
#define MUTEX_LOCK ;
//static mutex_lock_t mutex_lock;
/*将锁的id给用户，避免用户直接拿到数据结构*/
int mutex_id;
void lock_task1(void)
{
        int print_location = 5;
        while (1)
        {
                int i;
                if (!is_init)
                {

#ifdef MUTEX_LOCK
                        // mthread_mutex_init(&mutex_lock);
                        mutex_id = sys_lock_init();
#endif
                        is_init = TRUE;
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK1] Applying for a lock.\n");

                //sys_yield();

#ifdef MUTEX_LOCK
                // mthread_mutex_lock(&mutex_lock);
                sys_lock_jion(mutex_id, USER_OP_LOCK);
#endif

                for (i = 0; i < 20; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK1] Has acquired lock and running.(%d)\n", i);
                        //sys_yield();
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK1] Has acquired lock and exited.\n");

#ifdef MUTEX_LOCK
                // mthread_mutex_unlock(&mutex_lock);
                sys_lock_jion(mutex_id, USER_OP_UNLOCK);
#endif
                //sys_yield();
        }
}

void lock_task2(void)
{
        int print_location = 6;
        while (1)
        {
                int i;
                if (!is_init)
                {

#ifdef MUTEX_LOCK
                        // mthread_mutex_init(&mutex_lock);
                        mutex_id = sys_lock_init();
#endif
                        is_init = TRUE;
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK2] Applying for a lock.\n");
                //sys_yield();

#ifdef MUTEX_LOCK
                sys_lock_jion(mutex_id, USER_OP_LOCK);
#endif

                for (i = 0; i < 20; i++)
                {
                        sys_move_cursor(1, print_location);
                        printf("> [TASK2] Has acquired lock and running.(%d)\n", i);
                        //sys_yield();
                }

                sys_move_cursor(1, print_location);
                printf("%s", blank);

                sys_move_cursor(1, print_location);
                printf("> [TASK2] Has acquired lock and exited.\n");

#ifdef MUTEX_LOCK
                sys_lock_jion(mutex_id, USER_OP_UNLOCK);
#endif
                //sys_yield();
        }
}
