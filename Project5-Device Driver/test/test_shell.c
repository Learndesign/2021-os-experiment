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
#define COMMAND_NUM 6
#define True 1
#define False 0
#define SHELL_BEGIN 35
typedef void (*function)(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);
typedef void *parameter;

#define MAX_CHAR_SHELL 40               // shell单行最多可以输入的字符数
void clear_command(int *ptr);           //清空shell当前行的指令
void find_command(int *ptr);            //使用方向键时寻找保存的命令，加载到现在的命令
char buffer[MAX_CHAR_SHELL];            //用于保存现在的指令
#define MAX_CHAR_SHELL 40               // shell单行最多可以输入的字符数
char store_command[16][MAX_CHAR_SHELL]; //保存以往的指令
int buffer_ptr = 0;                     //用于指令buffer的指示
int store_row = 0;                      //用于store_command保存新的指令
int store_ptr = 0;                      //使用方向键往回找的指针

int parse_command(char *buffer, int length); //解析命令

static void shell_kill(int id);
static void shell_exec(const char *file_name, int argc, char *arg_3, char *arg_4, char *arg_5, char *arg_6);
static void shell_clear();
static void shell_ps();
static void shell_taskset(int mask, char *file_name, int mode);
static void shell_ls();

int main()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> stu: ");
    while (1)
    {
        // TODO: call syscall to read UART port
        char c = sys_get_char();
        char ch2;
        /*方向键,首位27是esc：换码字符*/
        if (c == 27)
        {
            ch2 = sys_get_char();
            /*换码后方向键的标志，↑： [A*/
            if (ch2 == '[')
            {
                ch2 = sys_get_char();
                /*判定上下左右键*/
                switch (ch2)
                {
                case 65:
                    store_ptr--;
                    // printf("store_row: %d\t", store_row);
                    if (store_ptr >= 0)
                    {
                        /*输入方向键的第一个字符是空格要舍弃*/
                        /*清除当前行内容，并且第一个空格也被清除，不做额外处理*/
                        clear_command(&buffer_ptr);
                        find_command(&buffer_ptr);
                    }
                    else
                    {
                        /*越界了需要把刚刚减下去的加回来*/
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
                        /*循环只做清空工作*/
                        clear_command(&buffer_ptr);
                    }
                    else
                    {
                        /*把刚刚减下去的加回来*/
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

            // store command
            for (int j = 0; j < buffer_ptr; j++)
            {
                store_command[store_row][j] = buffer[j];
            }
            store_row++;
            /*每次执行新的命令后往回找指针刷新为当前保存的行数*/
            store_ptr = store_row;

            // TODO: parse input
            // TODO: ps, exec, kill, clear
            parse_command(buffer, buffer_ptr);
            buffer_ptr = 0; //解析后清空
            printf("> stu: ");
        }
        else // note: backspace maybe 8('\b') or 127(delete)
            if (c == 8 || c == 127)
        {
            if (buffer_ptr) //避免删除终端提示符stu
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

int parse_command(char *buffer, int length)
{
    char argv[7][12] = {0};
    int i;
    int index = 0;
    /*判断是第几个参数*/
    int judge = 0;
    for (i = 0; i < length; i++)
    {
        char ch = buffer[i];
        if (ch == ' ')
        {
            argv[judge++][index++] = 0;
            index = 0;
        }
        else
            argv[judge][index++] = ch;
    }
    argv[judge][index] = 0;

    int exec = 0;
    int kill = 0;
    int ps = 0;
    int clear = 0;
    int ls = 0;
    if (!strcmp(argv[0], "exec"))
        exec = 1;
    if (!strcmp(argv[0], "kill"))
        kill = 1;
    if (!strcmp(argv[0], "ps"))
        ps = 1;
    if (!strcmp(argv[0], "clear"))
        clear = 1;
    if (!strcmp(argv[0], "ls"))
        ls = 1;
    parameter arg_1, arg_2, arg_3, arg_4, arg_5, arg_6;
    arg_1 = (parameter)argv[1];
    arg_2 = (parameter)argv[2];
    arg_3 = (parameter)argv[3];
    arg_4 = (parameter)argv[4];
    arg_5 = (parameter)argv[5];
    arg_6 = (parameter)argv[6];
    int argc = judge;
    if (exec)
    {
        shell_exec(arg_1, argc, arg_2, arg_3, arg_4, arg_5);
    }
    else if (ps)
    {
        shell_ps();
    }
    else if (clear)
    {
        shell_clear();
    }
    else if (kill)
    {
        if (!strcmp(argv[1], "all"))
        {
            for (int i = 2; i < 16; i++)
            {
                sys_kill(i);
            }
        }
        else
        {
            int pid = atoi(argv[1]);
            shell_kill(pid);
        }
    }
    else if (ls)
    {
        shell_ls();
    }
    else
    {
        printf("Unknown command\n");
        return;
    }

    sys_reflush();
    return 0;
}

static void shell_exec(const char *file_name, int argc, char *arg_3, char *arg_4, char *arg_5, char *arg_6)
{
    printf("exec process: %s\n", file_name);
    sys_reflush();
    char *argv[] = {
        file_name,
        arg_3,
        arg_4,
        arg_5,
        arg_6};
    pid_t pid = sys_exec(file_name, argc, argv, AUTO_CLEANUP_ON_EXIT);
    if (pid == -1)
    {
        printf("> [ERROR] : faild to create the process: %s\n", file_name);
    }
    else
    {
        printf("succeed to creat creat the process: %s, the pid: %d\n", file_name, pid);
    }
}

static void shell_clear()
{
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}

static void shell_ps()
{
    sys_ps();
}

static void shell_kill(int id)
{
    if (sys_kill((pid_t)id) != 0)
    {
        // printf("process[%d] has been kill successfuly!\n", id);
    }
    else
    {
        printf("[ERROR] process_%d is not exist\n", id);
    }
    sys_reflush();
}

static void shell_ls()
{
    sys_ls();
}

void clear_command(int *ptr)
{
    /*循环会新建同名局部变量，不能直接使用参数同名指针*/
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