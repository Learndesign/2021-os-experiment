#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>


ptr_t memCurr = FREEMEM;
ptr_t heapCurr = FREEHEAP;

uint64_t krand()
{
    uint64_t rand_base = get_ticks();
    long long tmp = 0x5deece66dll * rand_base + 0xbll;
    uint64_t result = tmp & 0x7fffffff;
    return result;
}

ptr_t allocPage()//分配一个页
{
    // TODO:
    /* 如果页表队列不为空，则需要重复利用 */
    if (!Is_empty_list(&freePageList)){
        free_page_t *new = get_entry_from_list(list_del(&freePageList), free_page_t, list);
        return new->kva + PAGE_SIZE;        
    }else{//否则需要现场分配一份资源
        memCurr += PAGE_SIZE;
        return memCurr;        
    }
}

void freePage(ptr_t baseAddr)
{
    // TODO:   
    free_page_t * free_page  = (free_page_t *)kmalloc(sizeof(free_page_t));
    free_page->kva = baseAddr;
    list_add(&free_page->list, &freePageList);
}

/* malloc a place in the heap */
void *kmalloc(size_t size)
{
    // TODO(if you need):
    ptr_t ret = ROUND(heapCurr, 4);
    heapCurr = ret + size;
    return (void*)ret;
}

uintptr_t shm_page_get(int key)
{
//     // TODO(c-core): 
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave;  
    if(!Is_empty_list(&ShareMemKey)){

        list_node_t * pointer = ShareMemKey.next;
        do{
            share_page * find = get_entry_from_list(pointer, share_page, list);
            if(find->key == key){
                uintptr_t vta;
repeat:         vta = krand() % 0xf0000000;
                vta &= ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
                if(!check_page_map(vta, current_running->pgdir))
                    goto repeat;
                else{
                    alloc_page_point_phyc(vta, current_running->pgdir, find->kva, MAP_USER);
                    find->visted++;
                    return vta ;
                }   
            }
            pointer = pointer->next;
        } while (pointer != &ShareMemKey);
    }
    /* alloc new */
    share_page * new = (share_page *)kmalloc(sizeof(share_page));
    new->key = key;
    uintptr_t kva;
    uintptr_t vta;
new:vta = krand() % 0x10000000;
    vta &= ((uint64_t)(~0) << NORMAL_PAGE_SHIFT);
    if(!check_page_map(vta, current_running->pgdir))
        goto new;
    else
        kva = alloc_page_helper(vta, current_running->pgdir);    
    new->kva = kva;
    new->visted = 1;
    list_add(&new->list, &ShareMemKey);
    return vta;    
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
    current_running = get_current_cpu_id() == 0 ? current_running_master :current_running_slave;  
    uintptr_t kva = get_pfn_of(addr, current_running->pgdir);
    if(!Is_empty_list(&ShareMemKey)){
        list_node_t * pointer = ShareMemKey.next;
        do{
            share_page * find = get_entry_from_list(pointer, share_page, list);
            if(find->kva == kva){
                if((--(find->visted)) == 0){
                    list_del_point(&find->list);
                    (*get_PTE_of(addr, current_running->pgdir)) = 0;
                    freePage(find->kva);
                }
            }
            pointer = pointer->next;
        } while (pointer != &ShareMemKey);        
    } else {
        prints("> [Error]\n");
        while (1);
    }
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    kmemcpy((char *)dest_pgdir, (char *)src_pgdir, PAGE_SIZE);
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
/**
 * 为虚拟地址制作一个映射到物理地址上随后将这个映射加入到页表pgdir当中
 * 返回这一页的虚拟地址 
 * 
 */

uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO:
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0)//unvalid
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&page_base[vpn[2]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to set the U, the kernel will not set the U 
         * which means that the User will not get it, but we will 
         * temporary set it as the User. so the U will be high
        */ 
        set_attribute(&page_base[vpn[2]], _PAGE_USER | _PAGE_PRESENT /*| _PAGE_ACCESSED | _PAGE_DIRTY*/ );
        /* assign second page_table */
        second_page = (PTE *) newpage; 
    }
    else
    {
        /* get the addr of the second_page */
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }
    
    /* third page */
    if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0 )//unvalid or the leaf
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&second_page[vpn[1]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to assign U to low */
        set_attribute(&second_page[vpn[1]], _PAGE_USER | _PAGE_PRESENT/*| _PAGE_ACCESSED | _PAGE_DIRTY*/ );
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
    if((third_page[vpn[0]] & _PAGE_PRESENT) != 0)
        /* return the physic page addr */
        return 0;//pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    uint64_t newpage = allocPage() - PAGE_SIZE;
    /* clear the physic page */
    clear_pgdir(newpage);
    set_pfn(&third_page[vpn[0]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
                        | _PAGE_EXEC /*| _PAGE_ACCESSED | _PAGE_DIRTY*/
                        | _PAGE_USER ;
    set_attribute(&third_page[vpn[0]], pte_flags);
    return newpage; 
}

/* 将虚拟地址进行映射，不过映射到的是指定的物理地址 */
PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint32_t mode){
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0)//unvalid
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&page_base[vpn[2]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to set the U, the kernel will not set the U 
         * which means that the User will not get it, but we will 
         * temporary set it as the User. so the U will be high
        */ 
        set_attribute(&page_base[vpn[2]],  (mode == MAP_USER ?
                                            _PAGE_USER : 0) | _PAGE_PRESENT);
        /* assign second page_table */
        second_page = (PTE *) newpage; 
    }
    else
    {
        /* get the addr of the second_page */
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }
    
    /* third page */
    if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0 )//unvalid or the leaf
    {
        /* alloc a new second page for the page */
        uint64_t newpage = allocPage() - PAGE_SIZE;
        /* clear the second page */
        clear_pgdir(newpage);
        set_pfn(&second_page[vpn[1]], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to assign U to low */
        set_attribute(&second_page[vpn[1]],(mode == MAP_USER ?
                                            _PAGE_USER : 0 ) | _PAGE_PRESENT);
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
    //     return 0;//pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    set_pfn(&third_page[vpn[0]], kva2pa(kva) >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
                        | _PAGE_EXEC | (mode == MAP_USER ?
                        _PAGE_USER : (_PAGE_ACCESSED | _PAGE_DIRTY)) ;
    set_attribute(&third_page[vpn[0]], pte_flags);
    return &third_page[vpn[0]];     
}

/* 检查改虚拟地址是否已经被映射 */
uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir){
    uint64_t vpn[] = {
                    (vta >> 12) & ~(~0 << 9), //vpn0
                    (vta >> 21) & ~(~0 << 9), //vpn1
                    (vta >> 30) & ~(~0 << 9)  //vpn2
                    };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0)//unvalid
    {
        return 1;
    }
    else
    {
        /* get the addr of the second_page */
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }
    
    /* third page */
    if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0 )//unvalid or the leaf
    {
        return 1;
    }
    else
    {
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
        /* the va's page */
    }

    /* final page */
    if((third_page[vpn[0]] & _PAGE_PRESENT) == 0)
        return 1;
    else
        return 0;
}

/* check the physic page in only without AD or in the SD */
uint32_t check_W_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode){
    uint64_t vpn[] = {
                    (va >> 12) & ~(~0 << 9), //vpn0
                    (va >> 21) & ~(~0 << 9), //vpn1
                    (va >> 30) & ~(~0 << 9)  //vpn2
                    };
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL; 
    /* physic page */
    PTE *physic_page = NULL;
    if (((page_base[vpn[2]]) & _PAGE_PRESENT)!=0)
    {
        // page_base[vpn[2]] |= _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0);
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return 0;

    if (((second_page[vpn[1]]) & _PAGE_PRESENT)!=0)
    {
        // second_page[vpn[1]] |= _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0);
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return 0; 
    /* 如果该页存在于SD卡中 */
    if(((third_page[vpn[0]]) & _PAGE_SD)!=0)
    {
        // third_page[vpn[0]] &= ~_PAGE_SD;//清空SD位
        if((third_page[vpn[0]] & _PAGE_WRITE) == 0 && mode == STORE){
            third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY;
            return 3;
        }
        third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0);
        return 2;
    }
    /* 检查是否为未分配页 */
    if (((third_page[vpn[0]]) & _PAGE_PRESENT)!=0)
    {
        if((third_page[vpn[0]] & _PAGE_WRITE) == 0 && mode == STORE){
            third_page[vpn[0]] |= _PAGE_PRESENT | _PAGE_ACCESSED |  _PAGE_DIRTY;
            return 4;
        }
        third_page[vpn[0]] |= (_PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0));
    }    
    else{
            return 0;
    }
    return 1;
}

