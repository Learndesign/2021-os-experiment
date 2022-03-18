#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>
#include <sbi.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__("sfence.vma"
                         :
                         :
                         : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__("sfence.vma %0"
                         :
                         : "r"(addr)
                         : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile("fence.i" ::
                     : "memory");
}

static inline void flush_icache_all(void)
{
    local_flush_icache_all();
    sbi_remote_fence_i(NULL);
}

static inline void flush_tlb_all(void)
{
    local_flush_tlb_all();
    sbi_remote_sfence_vma(NULL, 0, -1);
}
static inline void flush_tlb_page_all(unsigned long addr)
{
    local_flush_tlb_page(addr);
    sbi_remote_sfence_vma(NULL, 0, -1);
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0"
                         :
                         : "rK"(__v)
                         : "memory");
}

//内核页目录地址
#define PGDIR_PA 0x5e000000lu

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */
#define _PAGE_SD (1 << 9)       /* Clock SD flag */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t PTE;
//由内核虚拟地址转为物理地址
static inline uintptr_t kva2pa(uintptr_t kva)
{
    // TODO:
    /* mask == 0x0000000011111111 */
    uint64_t mask = (uint64_t)(~0) >> 32;
    /* 注意此时还不需要移动12位 */
    return kva & mask;
}

//物理地址到内核虚拟地址
static inline uintptr_t pa2kva(uintptr_t pa)
{
    // TODO:
    /* mask == 0xffffffc00000000 */
    uint64_t mask = (uint64_t)(~0) << 38;
    return pa | mask;
}

//转换得到PTE对应的物理地址
static inline uint64_t get_pa(PTE entry)
{
    // TODO:

    /* mask == 0031111111111111 */
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    /* ppn * 4096(0x1000) */
    return ((entry & mask) >> _PAGE_PFN_SHIFT) << 12;
}

/* Get/Set page frame number of the `entry` */
//得到物理页框号
static inline long get_pfn(PTE entry)
{
    // TODO:
    /*mask == 00c1111111111111*/
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    return (entry & mask) >> _PAGE_PFN_SHIFT;
}

//由页表得到va对应的物理地址，并将这个地址转换为内核虚拟地址返回
static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    uint64_t vpn[] = {
        (va >> 12) & ~(~0 << 9), //vpn0
        (va >> 21) & ~(~0 << 9), //vpn1
        (va >> 30) & ~(~0 << 9)  //vpn2
    };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << NORMAL_PAGE_SHIFT)));
}

static inline uintptr_t get_pfn_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * 该函数的功能是由页表得到va对应的物理页表的物理地址，最后将这个地址
     * 转换为内核虚拟地址返回
     */
    uint64_t vpn[] = {
        (va >> 12) & ~(~0 << 9), //vpn0
        (va >> 21) & ~(~0 << 9), //vpn1
        (va >> 30) & ~(~0 << 9)  //vpn2
    };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));
}

static inline PTE *get_PTE_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * 该函数的功能是由页表得到va对应的储存物理页表的表项，最后将这个地址
     * 转换为内核虚拟地址返回
     */
    uint64_t vpn[] = {
        (va >> 12) & ~(~0 << 9), //vpn0
        (va >> 21) & ~(~0 << 9), //vpn1
        (va >> 30) & ~(~0 << 9)  //vpn2
    };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    return &third_page[vpn[0]];
}

//将表项的ppn存到PTE指定位置中
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    // TODO:
    //先置为0在做或
    *entry &= (uint64_t)(~0) >> 54;
    //左移十位,避开十位标志位
    *entry |= (pfn << _PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
//得到PTE后十位指定位的的情况
static inline long get_attribute(PTE entry, uint64_t mask)
{
    // TODO:
    return entry & mask;
}

//设置PTE后面十位的标志位
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    // TODO:
    *entry |= bits;
}

//清空指定的页表，一页512项
static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    // TODO:
    uint8_t *pgdir = pgdir_addr;
    for (int i = 0; i < 512; i++)
        *pgdir++ = 0;
}

#endif // PGTABLE_H
