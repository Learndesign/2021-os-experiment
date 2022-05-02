#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>
#include <sbi.h>
#include <io.h>
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
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void flush_icache_all(void)
{
    local_flush_icache_all();
    sbi_remote_fence_i(NULL);
}

static inline void local_flush_dcache_all(void)
{
    dmb();
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
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define PGDIR_PA 0x5e000000lu  // use bootblock's page as PGDIR

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
#define _PAGE_SD   (1 << 9)     /* Clock SD flag */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t PTE;
static inline uintptr_t kva2pa(uintptr_t kva)
{
    // TODO:
    /**
     * 由虚拟地址得到真实地址
     */
    /* mask == 0x0000000011111111 */
    uint64_t mask = (uint64_t)(~0) >> 32;
    /* 注意此时还不需要移动12位 */
    return kva & mask ;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    // TODO:
    /**
     * 由真实地址得到虚拟地址
     */
    /* mask == 0xffffffc00000000 */
    uint64_t mask = (uint64_t)(~0) << 38;
    return pa | mask;
}


static inline uint64_t get_pa(PTE entry)
{
    // TODO:
    /**
     * 该函数的功能是在表项中得到真实的地址
     */
    /* mask == 0031111111111111 */
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    /* ppn * 4096(0x1000) */
    return ((entry & mask) >> _PAGE_PFN_SHIFT) << 12;
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    // TODO:
    /**
     * 得到pfn，但注意，此时得到的pfn还没有向左移动12位，即
     * 得到的是物理页表号
     * 即 * 4096(0x1000)
     */
    /*mask == 00c1111111111111*/
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    return (entry & mask) >> _PAGE_PFN_SHIFT; 
}

static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * 该函数的功能是由页表得到va对应的物理地址，最后将这个地址
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
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << NORMAL_PAGE_SHIFT)));
}

static inline uintptr_t get_pfn_of(uintptr_t va, uintptr_t pgdir_va){
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
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));    
}

static inline PTE * get_PTE_of(uintptr_t va, uintptr_t pgdir_va){
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
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    return &third_page[vpn[0]];    
}

static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    // TODO:
    /* 该函数的功能将表项的ppn存储到PTE的相应位中
     * 注意必须是向左移动十位，以避开功能位
     */
    *entry &= ((uint64_t)(~0) >> 54);
    *entry |= (pfn << _PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry)
{
    // TODO:
    /**
     * 得到PTE后十位的情况
     */

    uint64_t mask = (uint64_t)(~0) >> 54;
    return entry & mask;
}

static inline void set_attribute(PTE *entry, uint64_t bits)
{
    // TODO:
    /**
     * 该函数的功能是处理PTE的后十位，将其分别标志好，因此直接或
     */
    *entry &= ((uint64_t)(~0) << 10);
    *entry |= bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    // TODO:
    /**
     * 该函数的功能是将刚创建出的页表清空。
     */
    for(uint64_t i = 0; i < 0x1000; i+=8){
        *((uint64_t *)(pgdir_addr+i)) = 0;
    }
}

#endif  // PGTABLE_H
