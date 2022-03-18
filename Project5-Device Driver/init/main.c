/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
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

#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#include <assert.h>
#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
// #include <test.h>
#include <pgtable.h>
#include <os/lock.h>
#include <csr.h>
#include <os/ioremap.h>

extern void __global_pointer$();

extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[])
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));

    for (int i = 0; i < 32; i++)
    {
        pt_regs->regs[i] = 0;
    }

    if (pcb->type == USER_PROCESS || pcb->type == USER_THREAD)
    {
        pt_regs->regs[2] = (reg_t)user_stack;        //SP
        pt_regs->regs[3] = (reg_t)__global_pointer$; //GP
        pt_regs->regs[4] = (reg_t)pcb;               //TP
        pt_regs->sepc = entry_point;                 //sepc
        pt_regs->sstatus = SR_SUM;                   //sstatus
    }

    pt_regs->regs[10] = argc;
    pt_regs->regs[11] = argv;

    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);

    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;
    for (int i = 0; i < 14; i++)
    {
        st_regs->regs[i] = 0;
    }
    pcb->save_context = pt_regs;
    pcb->switch_context = st_regs;
    pcb->user_sp = user_stack;
    st_regs->regs[0] = (pcb->type == USER_PROCESS || pcb->type == USER_THREAD) ? (reg_t)&ret_from_exception : entry_point; //ra
    st_regs->regs[1] = pcb->kernel_sp;
    pcb->pge_num = 0;
    pcb->pge_h = NULL;
    pcb->clock_point = NULL;

    init_list(&pcb->wait_list);
    for (int i = 0; i < MAX_LOCK_NUM; i++)
    {
        pcb->handle_mutex[i] = -1;
    }
}
static void init_shell()
{
    pcb[0].pid = process_id++;
    pcb[0].pgdir = allocPage() - PAGE_SIZE;
    pcb[0].kernel_stack_base = allocPage();
    pcb[0].user_stack_base = USER_STACK_ADDR;
    //得到内核映射
    share_pgtable(pcb[0].pgdir, pa2kva(PGDIR_PA));
    //建立用户栈地址映射
    alloc_page_helper(pcb[0].user_stack_base - PAGE_SIZE, pcb[0].pgdir);
    pcb[0].used = 1;
    pcb[0].kernel_sp = pcb[0].kernel_stack_base;
    pcb[0].user_sp = pcb[0].user_stack_base;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    pcb[0].mask = 3;
    pcb[0].cursor_x = 1;
    pcb[0].cursor_y = 1;
    pcb[0].name = (char *)kmalloc(kstrlen(elf_files[0].file_name) * sizeof(char));
    pcb[0].origin = -1;
    kstrcpy(pcb[0].name, elf_files[0].file_name);
    ptr_t entry_point = load_elf(elf_files[0].file_content, *elf_files[0].file_length, pcb[0].pgdir, alloc_page_helper);
    /* init stack */
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, entry_point, &pcb[0], NULL, NULL);
    /* add it to ready queue */
    list_add(&pcb[0].list, &ready_queue);
    for (int i = 1; i < NUM_MAX_TASK; i++)
    {
        pcb[i].used = 0;
        pcb[i].origin = -1;
    }
    current_running_master = &pid0_pcb_master;
    current_running_slave = &pid0_pcb_slave;
}

//取消之前为boot.c运行映射的2ME
void cancel_boot_map()
{
    PTE *pgdir = (PTE *)pa2kva(PGDIR_PA);
    uint64_t pa_base = 0x50200000lu;
    uint64_t vpn2 = (pa_base >> 30) & ~(~0 << 9);
    uint64_t vpn1 = (pa_base >> 21) & ~(~0 << 9);
    PTE *pmd = (PTE *)pa2kva(get_pa(pgdir[vpn2]));
    pgdir[vpn2] = 0;
    pmd[vpn1] = 0;
}

static void init_syscall(void)
{
    for (int i = 0; i < NUM_SYSCALLS; i++)
        syscall[i] = (long int (*)()) & handle_other;
    /*睡眠*/
    syscall[SYSCALL_SLEEP] = &do_sleep;
    /*调度*/
    syscall[SYSCALL_YIELD] = &do_scheduler;

    /*时间*/
    syscall[SYSCALL_GET_TIMEBASE] = &get_timer;
    syscall[SYSCALL_GET_TICK] = &get_ticks;

    /*屏幕*/
    syscall[SYSCALL_GET_CHAR] = &sbi_console_getchar;
    syscall[SYSCALL_PUT_CHAR] = &screen_write_ch;
    syscall[SYSCALL_WRITE] = &screen_write;
    syscall[SYSCALL_CURSOR] = &screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = &screen_reflush;
    syscall[SYSCALL_DELETE] = &screen_delete;

    //锁
    syscall[SYSCALL_LOCK_OP] = &do_mutex_op;
    syscall[SYSCALL_LOCK_INIT] = &do_mutex_init;

    /*功能命令*/
    syscall[SYSCALL_GETPID] = &do_getpid;

    /*支持命令*/
    syscall[SYSCALL_PS] = &do_ps;
    syscall[SYSCALL_SCREEN_CLEAR] = &screen_clear;
    syscall[SYSCALL_EXEC] = &do_exec;
    syscall[SYSCALL_EXIT] = &do_exit;
    syscall[SYSCALL_KILL] = &do_kill;
    syscall[SYSCALL_WAITPID] = &do_waitpid;
    syscall[SYSCALL_LS] = &do_ls;

    /*信号量操作*/
    syscall[SYSCALL_SEM_INIT] = &do_semphore_init;
    syscall[SYSCALL_SEM_UP] = &do_semphore_up;
    syscall[SYSCALL_SEM_DOWN] = &do_semphore_down;
    syscall[SYSCALL_SEM_DESTORY] = &do_semphore_destory;

    /*屏障*/
    syscall[SYSCALL_BARRIER_INIT] = &do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = &do_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTORY] = &do_barrier_destory;

    /*信箱*/
    syscall[SYSCALL_MBOX_OPEN] = &do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = &do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = &do_mbox_send;
    syscall[SYACALL_MBOX_RECV] = &do_mbox_recv;
    syscall[SYSCALL_MBOX_SEND_RECV] = &do_mbox_send_recv;

    /*线程*/
    syscall[SYSCALL_THREAD_CREATE] = &do_thread_create;
    syscall[SYSCALL_THREAD_JION] = &do_thread_join;

    /* 网络驱动 */
    syscall[SYSCALL_NET_SEND] = &do_net_send;
    syscall[SYSCALL_NET_RECV] = &do_net_recv;
    syscall[SYSCALL_NET_IRQ] = &do_net_irq_mode;
}

/* 
 * 读取MAC各个寄存器组的基地址并做内核映射
 * plic（中断控制器）、slcr（系统级控制寄存器）
 * 实际上访问的都是ethernet 寄存器访问的时候使用 xemacps_config.BaseAddress这个虚地址
 */
void boot_first_core()
{
    // setup timebase
    // fdt_print(_dtb);
    // get_prop_u32(_dtb, "/cpus/cpu/timebase-frequency", &time_base);
    // time_base = sbi_read_fdt(TIMEBASE);
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    //plic 支持多少个外部 irq
    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
    uintptr_t _plic_addr =
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000 * NORMAL_PAGE_SIZE);
    // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    // xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);

    long status = EmacPsInit(&EmacPsInstance);
    printk("[EmacPsInstance.BaseAddr] virt:%lx \n\r", EmacPsInstance.Config.BaseAddress);
    printk("[plic_addr] phy:%lx \n\r", get_pfn_of(_plic_addr, pa2kva(PGDIR_PA)));
    printk("[slcr_bade_addr] phy %lx\n\r", get_pfn_of(XPS_SYS_CTRL_BASEADDR, pa2kva(PGDIR_PA)));
    printk("[EmacPsInstance.BaseAddr] the phyc address: %lx\n\r", get_pfn_of(EmacPsInstance.Config.BaseAddress, pa2kva(PGDIR_PA)));

    if (status != XST_SUCCESS)
    {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }

    // init futex mechanism
    // init_system_futex();
    // enable_interrupt();
    // net_poll_mode = 1;
    // xemacps_example_main();

    // init screen (QAQ)
    // screen_clear(0, SCREEN_HEIGHT - 1);
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    if (get_current_cpu_id() == 0)
    {
        // init Process Control Block (-_-!)S

        smp_init();
        boot_first_core();
        init_shell();
        printk("> [INIT] Shell initialization succeeded.\n\r");
        //while(1);

        // init all_array
        init_source_array();
        printk("> [INIT] All_Array initialization succeeded.\n\r");

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        screen_clear();
        wakeup_other_hart();
    }
    else
    {
        setup_exception();
    }

    if (get_current_cpu_id() == 0)
    {
        cancel_boot_map();
    }

    time_base = sbi_read_fdt(TIMEBASE);
    sbi_set_timer(get_ticks() + time_base / 100);

    while (1)
    {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler
        enable_interrupt();
        __asm__ __volatile__("wfi\n" ::
                                 :);
        //do_scheduler();
    }
    return 0;
}
