#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include <stdio.h>


static unsigned page_hash_func (const struct hash_elem *e, void *aux)
{
    struct page *p = hash_entry (e,struct page,page_elem);
    return (unsigned long long)(p->virtual_address) >> PGBITS;
}

static bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
    return page_hash_func (a,aux) < page_hash_func (b,aux);
}

void page_table_init (struct hash *supp_page_table)
{
    hash_init (supp_page_table,page_hash_func,page_less_func,0);
}

bool insert_page (struct hash *supp_page_table, struct page *p)
{
    struct hash_elem *e = hash_insert (supp_page_table,&p->page_elem);
    return (e == NULL);
}

bool delete_page (struct hash *supp_page_table, struct page *p)
{
    struct hash_elem *e = hash_delete (supp_page_table,&p->page_elem);
    if(p->physical_address != NULL){
        pagedir_clear_page(thread_current()->pagedir, p->virtual_address);
        palloc_free_page(p->physical_address);
    }
    free(p);
    return (e != NULL);
}

struct page *find_page (struct hash *supp_page_table, void *virtual_address)
{
    /*printf("[ page.c / find_page ] :: uva to find: %d\n",(int)virtual_address);*/
    struct page *p;
    p = malloc (sizeof(struct page));
    p->virtual_address = virtual_address;
    struct hash_elem *e =  hash_find (supp_page_table,&p->page_elem);
    free(p);
    if (e == NULL)
        return NULL;
    /*printf("[ page.c / find_page ] :: found uva : %d\n",(int)virtual_address);*/
    return hash_entry (e,struct page,page_elem);
}

void page_destroy_func (struct hash_elem *e, void *aux)
{
    struct page *p = hash_entry (e,struct page,page_elem);
    if(p->physical_address != NULL){
        pagedir_clear_page(thread_current()->pagedir, p->virtual_address);
        palloc_free_page(p->physical_address);
    }
    free(p);
}

void destroy_page_table (struct hash *supp_page_table)
{
    hash_destroy (supp_page_table,page_destroy_func);
}

//grow stack
struct page* grow_stack (void *ptr, void *esp)
{
    if (ptr < esp-32 || ptr >= PHYS_BASE
      || ptr < PHYS_BASE-0x00400000)
        return NULL;
    uint8_t *kpage = palloc_get_page (PAL_USER | PAL_ZERO);
    void *vpage;
    if (kpage == NULL)
        return NULL;
    vpage = (void*)(((int)ptr>>PGBITS)<<PGBITS);
    /*printf("{vm/page.c/grow_stack] :: vpage val : %d",(int)vpage);*/
    if (!install_page (vpage, kpage, true))
    {
        palloc_free_page (kpage);
        return NULL;
    }
    struct page *p = malloc (sizeof (struct page));
    if (p == NULL)
        return NULL;
    p->type = SWAP_PAGE;
    p->file = NULL;
    p->virtual_address = vpage;
    p->physical_address = kpage;
    p->writable = true;
    p->offset = 0;
    p->read_bytes = 0;
    p->zero_bytes = PGSIZE;
    p->swap_slot = -1;
    if(!insert_page (&thread_current ()->supp_page_table,p))
    {
        free(p);
        palloc_free_page (kpage);
        return NULL;
    }
    return p;
}
bool add_mmap_to_page_table(struct file *file, int32_t offset, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes){
    /*printf("[ page.c / add_mmap_to_page_table ] :: read_bytes : %d\n",read_bytes);*/
    struct page *p = malloc(sizeof(struct page));
    if(!p){
        return false;
    }
    p->file = file;
    p->offset = offset;
    p->virtual_address = upage;
    p->writable = true;
    p->read_bytes = read_bytes;
    p->zero_bytes = zero_bytes;
    p->type = MMAP_PAGE;
    if(!process_add_mmap(p)){
        /*printf("[ page.c / add_mmap_to_page_table ] :: fail\n");*/
        free(p);
        return false;
    }
    if (hash_insert(&thread_current()->supp_page_table, &p->page_elem)){
        return false;
    }
    /*printf("[ page.c / add_mmap_to_page_table ] :: success\n");*/
    return true;
}
