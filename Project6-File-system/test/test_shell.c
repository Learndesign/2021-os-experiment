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
#define COMMAND_NUM 15
#define True 1
#define False 0
#define SHELL_BEGIN 20
#define TYPE_FILE 1  
#define TYPE_DIR 2  
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

static void shell_kill(char *file_name);
static void shell_exec(const char *file_name, int argc, char *arg_3, char *arg_4, char *arg_5, char *arg_6);
static void shell_clear();
static void shell_ps();
static void shell_taskset(int mask, char *file_name, int mode);
static void shell_ls(char * unknown, char * path);
static void shell_mkfs();
static void shell_statfs();
static void shell_mkdir(char *name);
static void shell_rmdir(char *name);
static void shell_rm(char *name);
static void shell_cd(char *name);
static void shell_touch(char *name);
static void shell_cat(char *name);
static void shell_ln(char *source, char *name);
static struct
{
    char *name;
    function func;
    int arg_num;
} command_table[] = {
    {"ps", &shell_ps, 0},
    {"clear", &shell_clear, 0},
    {"exec", &shell_exec, 1},
    {"kill", &shell_kill, 1},
    {"taskset", &shell_taskset, 3},
    {"mkfs", &shell_mkfs, 0},
    {"statfs", &shell_statfs, 0},
    {"mkdir", &shell_mkdir, 1},
    {"rmdir", &shell_rmdir, 1},
    {"rm", &shell_rm, 1},
    {"cd", &shell_cd, 1},
    {"touch", &shell_touch, 1},
    {"cat", &shell_cat, 1},
    {"ls", &shell_ls, 0},
    {"ln", &shell_ln, 2}};

int main()
{
    // TODO:
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> stu: ");
    sys_reflush();
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
        printf("> [Error] faild to create the process: %s\n", file_name);
    }
    else
    {
        printf("succeed to create the process: %s, the pid: %d\n", file_name, pid);
    }
}

static void shell_clear()
{
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    sys_reflush();
}

static void shell_ps()
{
    sys_ps();
    sys_reflush();
}

static void shell_kill(char *file_name)
{
    pid_t pid = sys_kill(file_name);
    if (pid != -1)
    {
        printf("process: %s has been killed successfuly! pid: %d\n", file_name, pid);
    }
    else
    {
        printf("> [Error]\n");
    }
    sys_reflush();
}

static void shell_taskset(int mask, char *file_name, int mode)
{
    int pid = sys_taskset(file_name, mask, mode);
    if (mode == 0)
        printf("> taskset create the process: %s pid: %d as mask: %d successfully\n", file_name, pid, mask);
    else
        printf("> taskset change the process: %s pid: %d as mask: %d successfully\n", file_name, pid, mask);
}

//P6
static void shell_ls(char * unknown, char * path){
    if(!strcmp(unknown, "-l"))
        sys_ls(1, path);
    else
        sys_ls(0,unknown);
}

static void shell_mkfs()
{
    sys_mkfs();
}

static void shell_statfs()
{
    sys_statfs();
}

static void shell_mkdir(char *name)
{
    sys_mkdir(name);
}

static void shell_rmdir(char *name)
{
    sys_rmdir(name);
}

static void shell_rm(char *name)
{
    sys_rm(name);
}

static void shell_cd(char *name)
{
    sys_cd(name);
}

static void shell_touch(char *name)
{
    sys_touch(name);
}

static void shell_cat(char *name)
{
    sys_cat(name);
}

static void shell_ln(char*source,char *name)
{
    sys_ln(source,name);
}

int parse_command(char *buffer, int length)
{
    char cmd_two_dim[7][12] = {0};
    int i;
    int index = 0;
    int judge = 0;
    for (i = 0; i < length; i++)
    {
        char ch = buffer[i];
        if (ch == ' ')
        {
            cmd_two_dim[judge++][index++] = 0;
            index = 0;
        }
        else
            cmd_two_dim[judge][index++] = ch;
    }
    cmd_two_dim[judge][index] = 0;
    /*假设我们已经知道命令的数量*/
    for (i = 0; i < COMMAND_NUM; i++)
    {
        if (!strcmp(cmd_two_dim[0], command_table[i].name))
            break;
    }
    int taskset_create = 0;
    int taskset_change = 0;
    if (!strcmp(cmd_two_dim[0], "taskset"))
    { //taskset
        if (judge == 2)
            taskset_create = 1;
        else if (judge == 3)
            taskset_change = 1;
    }
    /* exec? */
    uint32_t exec = 0;
    if (!strcmp(cmd_two_dim[0], "exec"))
        exec = 1;
    if (i >= COMMAND_NUM)
    {
        printf("Unknown command\n");
        return;
    }
    else
    {
        parameter arg_1, arg_2, arg_3, arg_4, arg_5, arg_6;
        arg_1 = (parameter) cmd_two_dim[1];
        arg_2 = (parameter) cmd_two_dim[2];
        arg_3 = (parameter) cmd_two_dim[3];
        arg_4 = (parameter) cmd_two_dim[4];
        arg_5 = (parameter) cmd_two_dim[5];
        arg_6 = (parameter) cmd_two_dim[6];
        int argc = judge;
        if (taskset_create)
            arg_3 = 0;
        else if (taskset_change)
            arg_3 = 1;
        if (exec)
        {
            command_table[i].func(arg_1, argc,
                                  arg_2, arg_3,
                                  arg_4, arg_5);
        }
        else
        {
            command_table[i].func(arg_1, arg_2,
                                  arg_3, arg_4,
                                  arg_5, arg_6);
        }
    }
    return 0;
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