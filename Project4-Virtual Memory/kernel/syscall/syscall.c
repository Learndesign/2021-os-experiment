#include <os/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    // syscall[fn](arg1, arg2, arg3)
    /*发生例外的地址保存到sepc寄存器，异常前运行到的地址
     *返回时返回sepc+4地址
    */
    // screen_reflush();
    regs->sepc = regs->sepc + 4;
    /*将取出对应需要处理的系统调用函数进行操作并将返回值存在a0当中*/
    /*a0 = 系统调用函数的返回值，内核调用完毕后向用户返回*/
    /*内核调用的函数或许需要参数，因此传递a0,a1,a2*/
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],
                                              regs->regs[11],
                                              regs->regs[12],
                                              regs->regs[13]);
}
