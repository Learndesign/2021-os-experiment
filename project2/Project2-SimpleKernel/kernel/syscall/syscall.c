#include <os/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    // syscall[fn](arg1, arg2, arg3)
    regs->sepc = regs->sepc + 4;
    // a7存放系统调用类型，a0,a1,a2存放系统调用参数
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],
                                             regs->regs[11],
                                             regs->regs[12]);
}
