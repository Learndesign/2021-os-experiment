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
    pg_base += 0x1000;//4KB
    return pg_base;
}

// using 2MB large page
/* Sv39虚地址 [38:0]
 * vpn2[38:30]   vpn1[29:21]  vpn0[20:12]  offset[11:0]
 * 
 */
void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    va &= VA_MASK;//va_mask == 7fffffffff, 39个1, 通过 &= 得到后39位
    //虚地址向右移动30位即得到vpn2
    uint64_t vpn2 = 
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    //通过异或，使得得到vpn1，并将vpn2部分消为零
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    //if no second-level page directory
    if (pgdir[vpn2] == 0) {//分配一个二级页表
        // alloc a new second-level page directory
        // every second-level page need 4KB
        // 页表中存储的都是真实的地址
        // 分配一个页表，低12位一定为0
        uintptr_t newpage = alloc_page();
        set_pfn(&pgdir[vpn2], newpage >> NORMAL_PAGE_SHIFT);
        // 设置页表V位
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        // 清空新建的页表
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    /*内核二级页表真实的物理地址，除去低12的offset*/
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
/* Sv-39 mode
 * 为了向后兼容，因此地址为[63:39]要求是第38位的副本
 * 于是有了上述的分配方式
 */
void setup_vm()
{
    clear_pgdir(PGDIR_PA);//开始创建一个页表，每一个页表一开始都是空的，一个页表占4KB，因此首先全部清空为零
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    for (uint64_t kva = 0xffffffc050200000lu;/*每次向上增长2MB，初始化完成内核的所有页表项*/
         kva < 0xffffffc060000000lu; kva += 0x200000lu) {
        map_page(kva, kva2pa(kva), early_pgdir);
    } 
    // map boot address
    /*
     * 需要临时把 0x50201000 所在的 2MB 的页映射
     * 到 0x50201000 这个虚地址上，这样才能让 start 
     * 的代码在虚存开启的情况下也能正确运行。进入内核
     * 后需要记得取消
     */
    for (uint64_t pa = 0x50000000lu; pa < 0x50400000lu;
         pa += 0x200000lu) {
        map_page(pa, pa, early_pgdir);
    }
    /* 开启虚拟内存机制 */
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
    if (mhartid == 0) {//master kernel
        setup_vm();
        start_kernel =
            (kernel_entry_t)load_elf(_elf_main, _length_main,
                                     PGDIR_PA, directmap);
    } else {
        enable_vm();
    }
    start_kernel(mhartid, riscv_dtb);
    return 0;
}
