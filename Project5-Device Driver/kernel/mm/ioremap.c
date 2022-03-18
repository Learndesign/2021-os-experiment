#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    /* the addr of the kernel page */
    uint64_t kpgdir = pa2kva(PGDIR_PA);
    /* the num of needed page */
    int num_page = size / PAGE_SIZE + (size % PAGE_SIZE != 0);
    /* the pre va */
    uint64_t map_va = io_base & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
    /* the pointer phys addr */
    uint64_t map_kva = phys_addr & ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
    while((--num_page) >= 0){
        alloc_page_point_phyc(io_base, kpgdir, map_kva, MAP_KERNEL);
        io_base += PAGE_SIZE;
        map_kva += PAGE_SIZE;
    }
    local_flush_tlb_all();
    return (void *)map_va;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
