#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <emacps/xil_utils.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#include <plic.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

uintptr_t riscv_dtb;
uint64_t sd_place = 2500;

void write_sd(uintptr_t pa, uint64_t num, uint64_t sd_)
{
    // screen_move_cursor(1,15);
    // prints("> Attention: write %d sectors into the SD, the pa %lx\n", num, pa);
    sbi_sd_write(pa, num, sd_);
    screen_reflush();
}

void read_sd(uintptr_t pa, uint64_t num, uint64_t sd_)
{
    // screen_move_cursor(1,16);
    // prints("> Attention: read %d sectors into the mem, the pa %lx\n", num, pa);
    sbi_sd_read(pa, num, sd_);
    screen_reflush();
}

page_t *find_page(uintptr_t vta, pcb_t *find_pcb)
{
    // find_pcb = get_current_cpu_id() == 0 ? find_pcb_master :find_pcb_slave;
    list_node_t *sd_pointer = find_pcb->pge_h;
    do
    {
        page_t *sd_page = get_entry_from_list(sd_pointer, page_t, list);
        if (sd_page->vta == vta /*&& ((*sd_page->phyc_page_pte) & _PAGE_SD)*/)
            return sd_page;
        sd_pointer = sd_pointer->next;
    } while (sd_pointer != find_pcb->pge_h);

    prints("> [Error] failed to find a page in sd\n");
    screen_reflush();
    return NULL;
}

void clock_algorithm(page_t *new_page)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    // list_node_t * start = current_running->clock_point;
    page_t *select_page;
    PTE *select_pte;
    while (1)
    {
        select_page = get_entry_from_list(current_running->clock_point, page_t, list);
        select_pte = get_PTE_of(select_page->vta, current_running->pgdir);
        if ((*select_pte) & _PAGE_SD)
        {
            current_running->clock_point = current_running->clock_point->next;
            continue;
        }
        //如果AD位为高，则把相应位清零，时钟指针移动到下一个
        if (((*select_pte) & (_PAGE_ACCESSED | _PAGE_DIRTY)) != 0)
        {
            current_running->clock_point = current_running->clock_point->next;
            (*select_pte) &= ~(_PAGE_ACCESSED | _PAGE_DIRTY);
        }
        else
        {
            current_running->clock_point = current_running->clock_point->next;
            break;
        }
    };
    /* 将该页交换到SD卡中 */
    /* 清空V位 */
    if (((*select_pte) & _PAGE_WRITE) != 0)
        (*select_pte) &= ~(_PAGE_PRESENT);
    /* 设置in_sd位 */
    (*select_pte) |= _PAGE_SD;
    select_page->SD_place = sd_place;
    /* 如果选中的块没有打开W位 */
    if (((*select_pte) & _PAGE_WRITE) == 0)
    {
        uint32_t find_pid;
        pcb_t *find_pcb;
        find_pid = current_running->fork == -1 ? current_running->fork_son : current_running->fork;
        int i;
        for (i = 0; i < NUM_MAX_TASK; i++)
        {
            if (pcb[i].pid == find_pid)
            {
                find_pcb = &pcb[i];
                break;
            }
        }
        if (i >= NUM_MAX_TASK)
        {
            prints("> [Error]\n");
            while (1)
                ;
        }
        list_node_t *pointer = find_pcb->pge_h;
        do
        {
            page_t *pre = get_entry_from_list(pointer, page_t, list);
            if (pre->vta == select_page->vta)
            {
                PTE *find_pte = get_PTE_of(pre->vta, find_pcb->pgdir);
                // (*find_pte) &=  ~(_PAGE_PRESENT);
                (*find_pte) |= _PAGE_SD;
                pre->SD_place = sd_place;
                break;
            }
            pointer = pointer->next;
        } while (pointer != find_pcb->pge_h);
    }
    /* 写入SD卡 */
    write_sd(get_pfn_of(select_page->vta, current_running->pgdir), 8, sd_place);
    /* 如果该页确实存在于SD卡中 */
    sd_place += 8;
    PTE *new_pte = get_PTE_of(new_page->vta, current_running->pgdir);
    if ((*new_pte) & _PAGE_SD)
    {
        /* 清空SD位 */
        (*new_pte) &= ~(_PAGE_SD);
        /* 分配一个物理地址 */
        uintptr_t new_mem = get_pfn_of(select_page->vta, current_running->pgdir); //allocPage() - PAGE_SIZE;
        /* 从SD卡中读取8个扇区 */
        read_sd(kva2pa(new_mem), 8, new_page->SD_place);
        /* 重新设置表项 */
        set_pfn(new_pte, kva2pa(new_mem) >> NORMAL_PAGE_SHIFT);
        /* 置V位 */
    }
    else
    {
        /**
         * 否则代表这个页表项是新增的，需要加入循环链表，并且将他的空间拷贝给交换的
         */
        share_pgtable(get_pfn_of(select_page->vta, current_running->pgdir),
                      get_pfn_of(new_page->vta, current_running->pgdir));
        /* 回收这个页面 */
        freePage(get_pfn_of(new_page->vta, current_running->pgdir));
        set_pfn(new_pte, kva2pa(get_pfn_of(select_page->vta, current_running->pgdir)) >> NORMAL_PAGE_SHIFT);
        /* 插到前面 */
        list_node_t *add = current_running->clock_point;
        new_page->list.next = add;
        new_page->list.prev = add->prev;
        add->prev->next = &new_page->list;
        add->prev = &new_page->list;
    }
    local_flush_tlb_all();
}

/* 处理没有W位的情况 */
void deal_no_W(uintptr_t falut_addr)
{
    // prints("deal with no W\n");
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    pcb_t *father_pcb;
    pcb_t *son_pcb;
    if (current_running->fork == -1)
    {
        int i;
        for (i = 0; i < NUM_MAX_TASK; i++)
        {
            if (pcb[i].fork == current_running->pid)
            {
                son_pcb = &pcb[i];
                break;
            }
        }
        father_pcb = current_running;
        if (i >= NUM_MAX_TASK)
        {
            prints("> [Erroe] no this pid, the fork has wrong!\n");
            while (1)
                ;
        }
    }
    else
    {
        int i;
        for (i = 0; i < NUM_MAX_TASK; i++)
        {
            if (pcb[i].pid == current_running->fork)
            {
                father_pcb = &pcb[i];
                break;
            }
        }
        son_pcb = current_running;
        if (i >= NUM_MAX_TASK)
        {
            prints("> [Erroe] no this pid, the fork has wrong!\n");
            while (1)
                ;
        }
    }
    /* 为子进程分配新空间 */
    PTE *father_pte = get_PTE_of(falut_addr, father_pcb->pgdir);
    PTE *son_pte = get_PTE_of(falut_addr, son_pcb->pgdir);
    PTE *curr_pte = get_PTE_of(falut_addr, current_running->pgdir);
    uint64_t newpage = allocPage() - PAGE_SIZE;

    kmemcpy(newpage, get_pfn_of(falut_addr, current_running->pgdir), PAGE_SIZE);
    set_pfn(curr_pte, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    (*son_pte) |= _PAGE_WRITE;
    (*father_pte) |= _PAGE_WRITE;
    local_flush_tlb_all();
}

void check_net()
{
    /* recv queue */
    // current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    u32 reg_txsr;
    u32 reg_rxsr;

    if (!Is_empty_list(&net_recv_queue))
    {
        // screen_move_cursor(save_x, save_y);
        if ((reg_rxsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                                        XEMACPS_RXSR_OFFSET)) &
            (u32)XEMACPS_RXSR_FRAMERX_MASK)
        {
            /* write one to the FRAMERX to clear it */
            CLEANRXSR(&EmacPsInstance);
            /* unblock */
            do_unblock(net_recv_queue.prev);
        }

        /*flush cache*/
        Xil_DCacheFlushRange(0, 64);
    }
    /* send queue */
    if (!Is_empty_list(&net_send_queue))
    {
        //=========================================
        if ((reg_txsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                                        XEMACPS_TXSR_OFFSET)) &
            (u32)XEMACPS_TXSR_TXCOMPL_MASK)
        {
            /* write one to the COMPLE to clear it */
            CLEANTXSR(&EmacPsInstance);
            /* unblock */
            do_unblock(net_send_queue.prev);
            /*flush cache*/
            Xil_DCacheFlushRange(0, 64);
        }
    }
}

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    /*刷新屏幕，将sys_write的数据给显示在屏幕上*/
    screen_reflush();
    /*检查是否有任务到时间*/
    check_sleeping();

    /*检查网卡*/
    if (net_poll_mode == TIME_BREAK)
        check_net();
    /*设置下一个中断来临的时间点*/
    sbi_set_timer(get_ticks() + time_base / 200);
    /*在内核态调度*/
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    /*最高位为1，中断*/
    if ((cause & 0x8000000000000000) == 0x8000000000000000)
    {
        irq_table[regs->scause & 0x7fffffffffffffff](regs, stval, cause);
    }
    else
    {
        exc_table[regs->scause](regs, stval, cause);
    }
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    /* 重新设置时钟 */
    reset_irq_timer();
}

void handle_page_fault_inst(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    if (check_W_SD_and_set_AD(falut_addr, current_running->pgdir, LOAD) == ALLOC_NO_AD)
    {
        local_flush_tlb_all();
        return;
    }
    prints(">[Error] can't relove the inst page falut!\n");
    screen_reflush();
    while (1)
        ;
}

void handle_page_fault_load(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    /* load page fault */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    page_t *new_page;
    /* 如果只是分配了但是没有设置AD位 */
    uint32_t judge = check_W_SD_and_set_AD(falut_addr, current_running->pgdir, LOAD);
    if (judge == ALLOC_NO_AD)
    {
        local_flush_tlb_all();
        return;
    }
    else if (judge == IN_SD)
    /* 如果物理页被暂存在SD卡中 */
    {
        prints("> Attention: the 0x%lx addr was swap to SD! the origi phyc addr 0x%lx\n",
               falut_addr, kva2pa(get_pfn_of(falut_addr, current_running->pgdir)));
        // new_page = find_page(get_pfn_of(falut_addr, current_running->pgdir));
        new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT), current_running);
    }
    else
    {
        /* 否则需要分配一个物理页 */
        new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->vta = falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
        alloc_page_helper(falut_addr, current_running->pgdir);
        // new_page->phyc_page_pte = get_PTE_of(falut_addr, current_running->pgdir);//PTE表项的地址
        new_page->SD_place = 0;
        init_list(&new_page->list);
        if (current_running->pge_num < MAX_PAGE_NUM)
        {
            current_running->pge_num++;
            current_running->clock_point = &new_page->list;
            if (current_running->pge_h == NULL)
                current_running->pge_h = &new_page->list;
            else
                list_add(&new_page->list, current_running->pge_h);
            local_flush_tlb_all();
            return;
        }
        /* 如果持有的物理页数量已经到达上限 */
    }
    /* clock 替换策略 */
    clock_algorithm(new_page);
}

void handle_page_fault_store(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    /* load page fault */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    page_t *new_page;
    /* 如果只是分配了但是没有设置AD位 */
    uint32_t judge = check_W_SD_and_set_AD(falut_addr, current_running->pgdir, STORE);
    if (judge == ALLOC_NO_AD)
    {
        local_flush_tlb_all();
        return;
    }
    else if (judge == IN_SD)
    /* 如果物理页被暂存在SD卡中 */
    {
        prints("> Attention: the 0x%lx addr was swap to SD! the origi phyc addr 0x%lx\n",
               falut_addr, kva2pa(get_pfn_of(falut_addr, current_running->pgdir)));
        // new_page = find_page(get_pfn_of(falut_addr, current_running->pgdir));
        new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT), current_running);
    }
    else if (judge == IN_SD_NO_W)
    { //物理页在SD卡中并且没有W位
        new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT), current_running);
        /* 从SD卡中读取该页 */
        clock_algorithm(new_page);

        deal_no_W(falut_addr);

        return;
    }
    else if (judge == NO_W)
    { //物理页没有W位
        // new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT), current_running);
        deal_no_W(falut_addr);
        return;
    }
    else
    {
        /* 否则需要分配一个物理页 */
        new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->vta = falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
        alloc_page_helper(falut_addr, current_running->pgdir);
        // new_page->phyc_page_pte = get_PTE_of(falut_addr, current_running->pgdir);//PTE表项的地址
        new_page->SD_place = 0;
        init_list(&new_page->list);
        if (current_running->pge_num < MAX_PAGE_NUM)
        {
            current_running->pge_num++;
            current_running->clock_point = &new_page->list;
            if (current_running->pge_h == NULL)
                current_running->pge_h = &new_page->list;
            else
                list_add(&new_page->list, current_running->pge_h);
            local_flush_tlb_all();
            return;
        }
        /* 如果持有的物理页数量已经到达上限 */
    }
    /* clock 替换策略 */
    clock_algorithm(new_page);
}

void handle_irq(regs_context_t *regs, int irq)
{
    // TODO:
    // handle external irq from network device
    // let PLIC know that handle_irq has been finished
    /* fiqure the recv or send */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    u32 RegISR = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                                 XEMACPS_ISR_OFFSET);

    /* Clear the interrupt status register */
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET, RegISR);
    if (RegISR & XEMACPS_IXR_TXCOMPL_MASK)
    {
        if (!Is_empty_list(&net_send_queue))
            do_unblock(net_send_queue.prev);
        /* clear txsr if need */
        CLEANTXSR(&EmacPsInstance);
        Xil_DCacheFlushRange(0, 64);
    }
    /* wake up one recv */
    if (RegISR & XEMACPS_IXR_FRAMERX_MASK)
    {
        Xil_DCacheFlushRange(0, 64);
        XEmacPs_Bd *Recv_Bd = FindRxBd(&EmacPsInstance);
        if (!Is_empty_list(&net_recv_queue))
        {
            list_node_t *point = net_recv_queue.prev;
            do
            {
                pcb_t *recv_pcb = get_entry_from_list(point, pcb_t, list);
                list_node_t *del = point;
                point = point->prev;
                if (recv_pcb->produce == -1)
                {
                    do_unblock(del);
                    break;
                }
            } while (point != &net_recv_queue);
            // }
        }
        // if (!Is_empty_list(&net_recv_queue))
        // {
        //     list_node_t *point = net_recv_queue.prev;
        //     do
        //     {
        //         pcb_t *recv_pcb = get_entry_from_list(point, pcb_t, list);
        //         list_node_t *del = point;
        //         point = point->prev;
        //         /* normal */
        //         if (recv_pcb->produce == -1)
        //         {
        //             do_unblock(del);
        //             break;
        //         }
        //         /* choose port */
        //         if ((recv_pcb->pid % 2) && DATA == 0x51c3)
        //         {
        //             do_unblock(del);
        //             break;
        //         }
        //         else if ((recv_pcb->pid % 2 == 0) && DATA == 0x40e5)
        //         {
        //             do_unblock(del);
        //             break;
        //         }
        //     } while (point != &net_recv_queue);
        //     // }
        // }
        /* clear rxsr if need */
        CLEANRXSR(&EmacPsInstance);
        Xil_DCacheFlushRange(0, 64);
    }
    plic_irq_eoi(irq);
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    for (int i = 0; i < IRQC_COUNT; i++)
    {
        irq_table[i] = &handle_other;
    }
    for (int i = 0; i < EXCC_COUNT; i++)
    {
        exc_table[i] = &handle_other;
    }
    irq_table[IRQC_S_TIMER] = &handle_int;
    irq_table[IRQC_S_EXT] = &plic_handle_irq;
    exc_table[EXCC_SYSCALL] = &handle_syscall;
    exc_table[EXCC_LOAD_PAGE_FAULT] = &handle_page_fault_load;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_page_fault_store;
    exc_table[EXCC_INST_PAGE_FAULT] = &handle_page_fault_inst;
    /*将stvec寄存器设置好，并且打开中断使能*/
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char *reg_name[] = {
        "zero ", " ra  ", " sp  ", " gp  ", " tp  ",
        " t0  ", " t1  ", " t2  ", "s0/fp", " s1  ",
        " a0  ", " a1  ", " a2  ", " a3  ", " a4  ",
        " a5  ", " a6  ", " a7  ", " s2  ", " s3  ",
        " s4  ", " s5  ", " s6  ", " s7  ", " s8  ",
        " s9  ", " s10 ", " s11 ", " t3  ", " t4  ",
        " t5  ", " t6  "};
    for (int i = 0; i < 32; i += 3)
    {
        for (int j = 0; j < 3 && i + j < 32; ++j)
        {
            printk("%s : %016lx ", reg_name[i + j], regs->regs[i + j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000)
    {
        uintptr_t prev_ra = *(uintptr_t *)(fp - 8);
        uintptr_t prev_fp = *(uintptr_t *)(fp - 16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
