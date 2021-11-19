# UCAS 2021-2022 OSLab-Project

## 本实验采用 qemu+gdb 工具调试，PYNQ 开发板验证

- 汇编语言采用 RISCV 指令集，make 编译后即可验证


- 若需更换 kernel 需要在 makefile 内更改编译方式


 ## 实验内容：
- Project1主要目的仅为编写一个能加载kernel的启动器，因此kernel的内容比较简陋
  该启动器能实现大kernel启动
- Project2 实现简单支持时钟中断调度的的简单内核
- Project3 实现进程管理、mailbox通信与双核执行
- project4 实现操作系统的虚拟内存管理机制，包括虚实地址空间的管理， 换页机制
