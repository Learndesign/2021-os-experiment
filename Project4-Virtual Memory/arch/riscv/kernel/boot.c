/* RISC-V kernel boot stage */
#include <context.h>
#include <os/elf.h>
#include <pgtable.h>
#include <sbi.h>

typedef void (*kernel_entry_t)(unsigned long, uintptr_t);

extern unsigned char _elf_main[];
extern unsigned _length_main;

/********* setup memory mapping ***********/
uintptr_t alloc_page()
{
    static uintptr_t pg_base = PGDIR_PA;
    pg_base += 0x1000;
    return pg_base;
}

// using 2MB large page
//虚实地址建立映射
void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    //虚地址只有后面39位
    va &= VA_MASK;
    //右移30位得到vpn2
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    //把vpn2部分消去，右移拿到vpn1
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    if (pgdir[vpn2] == 0)
    {
        // alloc a new second-level page directory
        //页目录没有下一级页表，分配一页并设置页目录
        set_pfn(&pgdir[vpn2], alloc_page() >> NORMAL_PAGE_SHIFT); //一页4kb,只要页框号
        //设置页目录项V位为有效
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        //清空刚刚建立的二级页表
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    //拿到二级页表地址
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    //将物理页框号放在二级页表vpn1位置
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    /**
     * 要求在boot.c当中不能发生中断，因此在qeum当中若发现对应的A.D位
     * 为低，则发生中断，因此在此处直接将其置为1，并且置R，W为1，表示
     * 这是一个叶子节点，表示表示二级页表
     * 
     */
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
}

void enable_vm()
{
    // write satp to enable paging
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
void setup_vm()
{
    clear_pgdir(PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    for (uint64_t kva = 0xffffffc050200000lu;
         kva < 0xffffffc060000000lu; kva += 0x200000lu)
    {
        map_page(kva, kva2pa(kva), early_pgdir);
    }

    // map boot address
    // 这里需要额外把虚地址0x50000000 ~0x50400000映射到
    // 物理地址0x50000000~0x50400000。因为我们这个boot.c当前的
    // 地址在这个范围内。一旦开启虚存，所有的访存都会被认为是虚地址，
    // 所以为了boot.c能够正常运行完成，需要临时做一下这个映射。
    // 到内核正确启动后，由内核取消这个映射。
    for (uint64_t pa = 0x50000000lu; pa < 0x50400000lu; pa += 0x200000lu)
    {
        map_page(pa, pa, early_pgdir);
    }
    /* 开虚存 */
    enable_vm();
}

uintptr_t directmap(uintptr_t kva, uintptr_t pgdir)
{
    // ignore pgdir
    return kva;
}

kernel_entry_t start_kernel = NULL;

/*********** start here **************/
int boot_kernel(unsigned long mhartid, uintptr_t riscv_dtb)
{
    //主核
    if (mhartid == 0)
    {
        setup_vm();
        start_kernel =
            (kernel_entry_t)load_elf(_elf_main, _length_main,
                                     PGDIR_PA, directmap);
    }
    else
    {
        enable_vm();
    }
    start_kernel(mhartid, riscv_dtb);
    return 0;
}
