/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};

struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};
struct task_info task_test_multimaibox = {(uintptr_t)&test_multibox, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task,
                                           &strgenerator_task,
                                           &task_test_multimaibox,
                                           &task_test_affinity};
static int num_test_tasks = 8;

#define MAX_CHAR_SHELL 40               //shell????????????????????????????????????
#define SHELL_BEGIN 27                  //shell????????????????????????
void kill_process(pid_t num);           //??????pid???num?????????
void exec_process(int num);             //??????task??????num????????????0<num<16
void parse_command(char buffer[]);      //??????????????????????????????
void pintf_all_command();               //help??????????????????????????????
void clear_command(int *ptr);           //??????shell??????????????????
void find_command(int *ptr);            //??????????????????????????????????????????????????????????????????
char buffer[MAX_CHAR_SHELL];            //???????????????????????????
char getchar_uart();                    //????????????????????????getchar
char store_command[16][MAX_CHAR_SHELL]; //?????????????????????
int buffer_ptr = 0;                     //????????????buffer?????????
int store_row = 0;                      //??????store_command??????????????????
int store_ptr = 0;                      //?????????????????????????????????

void shell_taskset(int mask, int id, int mode);

void test_shell()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> stu: ");
    while (1)
    {
        // TODO: call syscall to read UART port
        char c = getchar_uart();
        char ch2;
        /*?????????,??????27???esc???????????????*/
        if (c == 27)
        {
            ch2 = getchar_uart();
            /*???????????????????????????????????? [A*/
            if (ch2 == '[')
            {
                ch2 = getchar_uart();
                /*?????????????????????*/
                switch (ch2)
                {
                case 65:
                    store_ptr--;
                    // printf("store_row: %d\t", store_row);
                    if (store_ptr >= 0)
                    {
                        /*???????????????????????????????????????????????????*/
                        /*??????????????????????????????????????????????????????????????????????????????*/
                        clear_command(&buffer_ptr);
                        find_command(&buffer_ptr);
                    }
                    else
                    {
                        /*?????????????????????????????????????????????*/
                        store_ptr++;
                    }
                    break;
                case 66:
                    store_ptr++;
                    // printf("store_row: %d\t", store_row);
                    if (store_ptr < store_row)
                    {
                        clear_command(&buffer_ptr);
                        find_command(&buffer_ptr);
                    }
                    else if (store_ptr == store_row)
                    {
                        /*????????????????????????*/
                        clear_command(&buffer_ptr);
                    }
                    else
                    {
                        /*??????????????????????????????*/
                        store_ptr--;
                    }
                    break;
                case 67:
                    clear_command(&buffer_ptr);
                    printf("The key you Pressed is : < direction \n");
                    printf("> stu: ");
                    break;
                case 68:
                    clear_command(&buffer_ptr);
                    printf("The key you Pressed is : < direction \n");
                    printf("> stu: ");
                    break;
                default:
                    clear_command(&buffer_ptr);
                    printf("No direction keys detected \n");
                    printf("> stu: ");
                    break;
                }
            }
        }
        else if (c == '\r')
        { //'Enter'
            printf("\n");
            buffer[buffer_ptr] = '\0';

            //store command
            for (int j = 0; j < buffer_ptr; j++)
            {
                store_command[store_row][j] = buffer[j];
            }
            store_row++;
            /*????????????????????????????????????????????????????????????????????????*/
            store_ptr = store_row;

            // TODO: parse input
            // TODO: ps, exec, kill, clear
            parse_command(buffer);
            buffer_ptr = 0; //???????????????
            printf("> stu: ");
        }
        else // note: backspace maybe 8('\b') or 127(delete)
            if (c == 8 || c == 127)
        {
            if (buffer_ptr != 0) //???????????????????????????stu
            {
                buffer_ptr--;
                sys_delete();
            }
        }
        else
        {
            if (buffer_ptr < MAX_CHAR_SHELL)
            {
                buffer[buffer_ptr++] = c;
                printf("%c", c);
            }
        }
    }
}

void pintf_all_command()
{
    printf("[1] ps : show all process with their status\n");
    printf("[2] clear : clear current screen\n");
    printf("[3] exec i : start task i and 0 < i < 16\n");
    printf("[4] kill i : kill a process and it's pid is i\n");
    printf("[5] help: look up all command\n");
    printf("[6] taskset 0xn i : start i task and set it in n core to run.");
    printf("[7] taskset -p 0xn i : change process_i to run in n core  ");
};

void parse_command(char buffer[])
{
    /*???????????????*/
    char argv[2][8];
    /*??????exce i????????????i*/
    int num;
    int mode;
    int i = 0, j = 0;
    while (buffer[j] != '\0' && buffer[j] != ' ')
    {
        argv[0][i++] = buffer[j++];
    }
    argv[0][i] = '\0';

    if (!strcmp(argv[0], "taskset"))
    {
        int count_arg = 0;

        int k = j;
        while (buffer[k++] != '\0')
        {
            i = 0;
            count_arg++;
            while (buffer[k] != '\0' && buffer[k] != ' ')
            {
                argv[count_arg][i++] = buffer[k++];
            }
            argv[count_arg][i] = '\0';
        }
        if (!strcmp(argv[1], "-p"))
        { // taskset -p mask pid
            //???????????? pid ???????????????????????? mask
            int mask, pid;
            if (!strcmp(argv[2], "0x1"))
            {
                mask = 1;
            }
            else if (!strcmp(argv[2], "0x2"))
            {
                mask = 2;
            }
            else
            {
                mask = 3;
            }
            pid = atoi(argv[3]);
            mode = 1;
            shell_taskset(mask, pid, mode);
        }
        else
        { // taskset mask num
            // ????????????????????????????????????????????? mask
            int mask;
            if (!strcmp(argv[1], "0x1"))
            {
                mask = 1;
            }
            else if (!strcmp(argv[1], "0x2"))
            {
                mask = 2;
            }
            else
            {
                mask = 3;
            }
            num = atoi(argv[2]);
            mode = 0;
            shell_taskset(mask, num, mode);
        }
        return;
    }

    if (buffer[j++] == ' ')
    {
        i = 0;
        while (buffer[j] != '\0')
        {
            argv[1][i++] = buffer[j++];
        }
        argv[1][i] = '\0';
        num = atoi(argv[1]);
    }

    int op_ps = (strcmp(argv[0], "ps") == 0);
    int op_clear = (strcmp(argv[0], "clear") == 0);
    int op_kill = (strcmp(argv[0], "kill") == 0);
    int op_exec = (strcmp(argv[0], "exec") == 0);
    int op_help = (strcmp(argv[0], "help") == 0);

    if (op_help)
    {
        pintf_all_command();
    }
    else if (op_ps)
    {
        sys_ps();
    }
    else if (op_exec)
    {
        exec_process(num);
    }
    else if (op_kill)
    {
        kill_process(num);
    }
    else if (op_clear)
    {
        sys_clear();
        sys_move_cursor(1, SHELL_BEGIN);
        printf("------------------- COMMAND -------------------\n");
    }
    else
    {
        printf("[ERROR]: Unknown command!  - try 'help'\n");
    }
}

void shell_taskset(int mask, int id, int mode)
{
    int pid = sys_taskset(test_tasks[id - 1], mask, id, mode);
    if (mode == 0)
        printf("> taskset create the process[%d]  as mask: %d successfully\n", pid, mask);
    else
        printf("> taskset change the process[%d]  as mask: %d successfully\n", id, mask);
}

void exec_process(int num)
{
    if (num > 0 && num < 16)
    {
        pid_t pid;
        pid = sys_spawn(test_tasks[num - 1], NULL, AUTO_CLEANUP_ON_EXIT);
        printf("successfully exec task_%d, it's pid: %d\n", num, pid);
    }
    else
    {
        printf("[Warning]: task_num should 0< task_num < 16\n");
    }
}

void kill_process(pid_t num)
{
    int killed;
    if (num > 0 && num <= 16)
    {
        killed = sys_kill(num);
        if (killed == 1)
        {
            printf("process[%d] has been killed.\n", num);
        }
        else if (killed == 2)
        {
            printf("[ERROR]: shell can not be killed\n");
        }
        else
        {
            printf("[ERROR]: process[%d] is not running.\n", num);
        }
    }
    else
    {
        printf("[ERROR]: process_%d not exit\n", num);
    }
}

char getchar_uart()
{
    char c;
    int n;
    while ((n = sys_read()) == -1)
        ;
    c = (char)n;
    return c;
}

void clear_command(int *ptr)
{
    /*????????????????????????????????????????????????????????????????????????*/
    int i = *ptr;
    while (i)
    {
        sys_delete();
        i--;
    }
    *ptr = i;
}

void find_command(int *ptr)
{
    int i = *ptr;
    for (int k = 0; k < MAX_CHAR_SHELL; k++)
    {
        buffer[k] = store_command[store_ptr][k];
        if (buffer[k] == 0)
            break;
        i++;
        printf("%c", buffer[k]);
    }
    *ptr = i;
}