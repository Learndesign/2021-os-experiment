# Project3 : *[process](javascript:;)* *[management](javascript:;)* 

### 概览任务：


| Level  |                 Task                 |
| :----: | :----------------------------------: |
| S-core |  shell终端、shell命令、barrier原语   |
| A-core | semaphore原语，mailbox通信，双核支持 |
| C-core |   双核可同时收发信箱，taskset指令    |

### shell支持指令：

|     Instruction     |                Function                |      Format      |
| :-----------------: | :------------------------------------: | :--------------: |
|        kill         |          杀死pid 为 i 的进程           |      kill i      |
|         ps          |            显示当前进程情况            |        ps        |
|        exec         |      启动test中第i个任务（0<i<9）      |      exec i      |
|        help         |           查看所有支持的指令           |       help       |
|  Up and down keys   |        支持回溯寻找使用过的指令        |    键盘上下键    |
|  taskset mask num   | 启动任务num并设置其允许运行的核为 mask | taskset 0xn num  |
| taskset -p mask pid |   设置进程 pid 的允许运行的核为 mask   | taskset -p 0xn i |

### 测试任务：

```c
static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task,
                                           &strgenerator_task,
                                           &task_test_multimaibox,
                                           &task_test_affinity};
```

为便于用户正常使用，测试任务号默认从1开始，exec 1将启动waitpid任务
