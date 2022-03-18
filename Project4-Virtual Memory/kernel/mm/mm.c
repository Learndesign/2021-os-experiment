#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>

ptr_t memCurr = FREEMEM;
ptr_t heapCurr = FREEHEAP;

//分配一页
ptr_t allocPage()
{
    // TODO:
    /* 如果页表队列不为空，则需要重复利用 */
    if (!list_empty(&freePageList))
    {
        free_page_t *new = list_entry(get_first_node(&freePageList), free_page_t, list);
        return new->kva + PAGE_SIZE;
    }
    else
    {
        memCurr += PAGE_SIZE;
        return memCurr;
    }
}

void freePage(ptr_t baseAddr)
{
    // TODO:
    free_page_t *free_page = (free_page_t *)kmalloc(sizeof(free_page_t));
    free_page->kva = baseAddr;
    list_add(&free_page->list, &freePageList);
}

/* malloc a place in the heap */
void *kmalloc(size_t size)
{
    // TODO(if you need):
    ptr_t ret = ROUND(heapCurr, 4);
    heapCurr = ret + size;
    return (void *)ret;
}

uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
}

/* this is used for mapping kernel virtual address into user page table */
//映射内核虚地址到用户页表
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    kmemcpy((char *)dest_pgdir, (char *)src_pgdir, PAGE_SIZE);
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */

//给虚拟地址分配一个物理地址并填表
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO:
    uint64_t vpn[] = {
        (va >> 12) & ~(~0 << 9), //vpn0
        (va >> 21) & ~(~0 << 9), //vpn1
        (va >> 30) & ~(~0 << 9)  //vpn2
    };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *)pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0) //unvalid
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&page_base[vpn[2]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        set_attribute(&page_base[vpn[2]], _PAGE_USER | _PAGE_PRESENT /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
        /* assign second page_table */
        second_page = (PTE *)newpage;
    }
    else
    {
        /* get the addr of the second_page */
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }

    /* third page */
    if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0) //unvalid or the leaf
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&second_page[vpn[1]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to assign U to low */
        set_attribute(&second_page[vpn[1]], _PAGE_USER | _PAGE_PRESENT /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
        /* the final page which it the finally page*/
        third_page = (PTE *)newpage;
        /* the va's page */
    }
    else
    {
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
        /* the va's page */
    }

    /* final page */
    // if((third_page[vpn[0]] & _PAGE_PRESENT) != 0)
    //     /* return the physic page addr */
    //     return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    uint64_t newpage = allocPage() - PAGE_SIZE;
    /* clear the physic page */
    clear_pgdir(newpage);
    set_pfn(&third_page[vpn[0]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC /*| _PAGE_ACCESSED | _PAGE_DIRTY*/
                         | _PAGE_USER;
    set_attribute(&third_page[vpn[0]], pte_flags);
    return newpage;
}
/* check the physic page in only without AD or in the SD */
uint32_t check_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode)
{
    uint64_t vpn[] = {
        (va >> 12) & ~(~0 << 9), //vpn0
        (va >> 21) & ~(~0 << 9), //vpn1
        (va >> 30) & ~(~0 << 9)  //vpn2
    };
    PTE *page_base = (uint64_t *)pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* physic page */
    PTE *physic_page = NULL;
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) != 0)
    {
        set_attribute(&page_base[vpn[2]], _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY : 0));
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }
    else
        return 0;

    if (((second_page[vpn[1]]) & _PAGE_PRESENT) != 0)
    {
        set_attribute(&second_page[vpn[1]], _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY : 0));
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    }
    else
        return 0;
    /* 如果该页存在于SD卡中 */
    if (((third_page[vpn[0]]) & _PAGE_SD) != 0)
    {
        // third_page[vpn[0]] &= ~_PAGE_SD;//清空SD位
        set_attribute(&third_page[vpn[0]], _PAGE_PRESENT | _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY : 0));
        return 2;
    }
    /* 检查是否为未分配页 */
    if (((third_page[vpn[0]]) & _PAGE_PRESENT) != 0)
    {
        set_attribute(&third_page[vpn[0]], _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY : 0));
    }
    else
    {
        return 0;
    }
    return 1;
}
