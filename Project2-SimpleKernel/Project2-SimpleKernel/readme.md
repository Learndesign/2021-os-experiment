# Project2: SimpleKernel

* ### 基本信息

|        |       |                   实现功能                   |
| ------ | :---: | :------------------------------------------: |
| S-core | Task3 |      非抢占式调度，锁，进入例外打印报错      |
| A-core | Task4 | 带内核态保护的系统调用，时钟中断处理，抢占式 |
| C-core | Task5 |               优先级调度，fork               |



* ### 使用方法

  在Project2-SimpleKernel\init\main.c 中static void init_pcb() 前define TASK3，TASK4，TASK5或者TASKP2（包含3和4）后make即可执行不同任务

