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
uint64_t sd_place = 1500;

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

page_t *find_page(uintptr_t vta)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    list_node_t *sd_pointer = current_running->pge_h;
    do
    {
        page_t *sd_page = list_entry(sd_pointer, page_t, list);
        if (sd_page->vta == vta && ((*sd_page->phyc_page_pte) & _PAGE_SD))
            return sd_page;
        sd_pointer = sd_pointer->next;
    } while (sd_pointer != current_running->pge_h);
    prints("> [ERROR] : failed to find a page in sd\n");
    // while(1);
}

void clock_algorithm(page_t *new_page)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    // list_node_t * start = current_running->clock_point;
    page_t *select_page;
    /* 选择一页替换出去 */
    while (1)
    {
        select_page = list_entry(current_running->clock_point, page_t, list);
        if ((*select_page->phyc_page_pte) & _PAGE_SD)
        {
            current_running->clock_point = current_running->clock_point->next;
            continue;
        }
        if (((*select_page->phyc_page_pte) & (_PAGE_ACCESSED | _PAGE_DIRTY)) != 0)
        {
            current_running->clock_point = current_running->clock_point->next;
            *select_page->phyc_page_pte &= ~(_PAGE_ACCESSED | _PAGE_DIRTY);
        }
        else
        {
            current_running->clock_point = current_running->clock_point->next;
            break;
        }
    };
    (*select_page->phyc_page_pte) &= ~(_PAGE_PRESENT);
    (*select_page->phyc_page_pte) |= _PAGE_SD;
    select_page->SD_place = sd_place;
    write_sd(kva2pa(select_page->kva), 8, sd_place);
    sd_place += 8;
    /*  需要页在SD卡就读出  */
    if ((*new_page->phyc_page_pte) & _PAGE_SD)
    {
        (*new_page->phyc_page_pte) &= ~(_PAGE_SD);
        uintptr_t new_mem = select_page->kva; // allocPage() - PAGE_SIZE;
        read_sd(kva2pa(new_mem), 8, new_page->SD_place);
        new_page->kva = new_mem;
        set_pfn(new_page->phyc_page_pte, kva2pa(new_mem) >> NORMAL_PAGE_SHIFT);
    }
    else
    {
        share_pgtable(select_page->kva, new_page->kva);
        freePage(new_page->kva);
        new_page->kva = select_page->kva;
        set_pfn(new_page->phyc_page_pte, kva2pa(new_page->kva) >> NORMAL_PAGE_SHIFT);
        list_node_t *add = current_running->clock_point;
        new_page->list.next = add;
        new_page->list.prev = add->prev;
        add->prev->next = &new_page->list;
        add->prev = &new_page->list;
    }
    local_flush_tlb_all();
}

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    screen_reflush();
    timer_check();
    /*检查网卡*/
    if (net_poll_mode == TIME_BREAK)
        check_net();
    sbi_set_timer(get_ticks() + time_base / 200);
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
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
    reset_irq_timer();
}

void handle_page_fault_inst(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    if (check_SD_and_set_AD(falut_addr, current_running->pgdir, LOAD) == ALLOC_NO_AD)
        return;
    prints(">[ERROR] : can't relove the inst page falut!\n");
    screen_reflush();
    while (1)
        ;
}

void handle_page_fault_load(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    /* load page fault */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    uint64_t get_phyc_page;
    PTE *get_phyc_PTE = NULL;
    page_t *new_page;
    /* 如果只是分配了但是没有设置AD位 check_SD_and_set_AD中设置AD位 */
    uint32_t judge = check_SD_and_set_AD(falut_addr, current_running->pgdir, LOAD);
    if (judge == ALLOC_NO_AD)
    {
        local_flush_tlb_all();
        return;
    }
    else if (judge == IN_SD)
    /* 如果物理页被暂存在SD卡中 */
    {
        prints("> Attention: the 0x%lx addr was swap to SD! the original phyc addr 0x%lx\n",
               falut_addr, kva2pa(get_pfn_of(falut_addr, current_running->pgdir)));
        new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT));
    }
    else
    {
        /* 否则需要分配一个物理页 */
        get_phyc_page = alloc_page_helper(falut_addr, current_running->pgdir);
        get_phyc_PTE = get_PTE_of(falut_addr, current_running->pgdir); // PTE表项的地址
        new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->vta = falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
        new_page->kva = get_phyc_page;
        new_page->phyc_page_pte = get_phyc_PTE;
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
    }
    clock_algorithm(new_page);
}

void handle_page_fault_store(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    /* load page fault */
    current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    uint64_t falut_addr = stval;
    uint64_t get_phyc_page;
    PTE *get_phyc_PTE = NULL;
    page_t *new_page;
    /* 如果只是分配了但是没有设置AD位 */
    uint32_t judge = check_SD_and_set_AD(falut_addr, current_running->pgdir, STORE);
    if (judge == ALLOC_NO_AD)
    {
        local_flush_tlb_all();
        return;
    }
    else if (judge == IN_SD)
    /* 如果物理页被暂存在SD卡中 */
    {
        prints("> Attention: the 0x%lx addr was swap to SD! the original phyc addr 0x%lx\n",
               falut_addr, kva2pa(get_pfn_of(falut_addr, current_running->pgdir)));
        // new_page = find_page(get_pfn_of(falut_addr, current_running->pgdir));
        new_page = find_page(falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT));
    }
    else
    {
        /* 否则需要分配一个物理页 */
        get_phyc_page = alloc_page_helper(falut_addr, current_running->pgdir);
        get_phyc_PTE = get_PTE_of(falut_addr, current_running->pgdir); // PTE表项的地址
        new_page = (page_t *)kmalloc(sizeof(page_t));
        new_page->vta = falut_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
        new_page->kva = get_phyc_page;
        new_page->phyc_page_pte = get_phyc_PTE;
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

//P5
void check_net()
{
    /* recv queue */
    // current_running = get_current_cpu_id() == 0 ? current_running_master : current_running_slave;
    u32 reg_txsr;
    u32 reg_rxsr;

    if (!list_empty(&net_recv_queue))
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
    if (!list_empty(&net_send_queue))
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
        if (!list_empty(&net_send_queue))
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
        if (!list_empty(&net_recv_queue))
        {
            list_node_t *point = net_recv_queue.prev;
            do
            {
                pcb_t *recv_pcb = list_entry(point, pcb_t, list);
                list_node_t *del = point;
                point = point->prev;
                do_unblock(del);
                break;
            } while (point != &net_recv_queue);
            // }
        }
        // if (!list_empty(&net_recv_queue))
        // {
        //     list_node_t *point = net_recv_queue.prev;
        //     do
        //     {
        //         pcb_t *recv_pcb = list_entry(point, pcb_t, list);
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
