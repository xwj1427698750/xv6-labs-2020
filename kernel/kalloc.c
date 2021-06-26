// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
    int ref_count[PGROUNDUP(PHYSTOP-KERNBASE) / PGSIZE];
} kmem;


/** 更新page_ref有锁*/
void incr_page_ref(uint64 idx, int val) {
    acquire(&kmem.lock);
    kmem.ref_count[(idx-KERNBASE) / PGSIZE] += val;
    release(&kmem.lock);
}

/** 更新page_ref*/
void incr_page_ref_no_lock(uint64 idx, int val) {
    kmem.ref_count[(idx-KERNBASE) / PGSIZE] += val;
}
void set_page_ref(uint64 idx, int val) {
    kmem.ref_count[(idx-KERNBASE) / PGSIZE] = val;
}
/** 获取page_ref*/
int get_page_ref(uint64 idx) {
    return kmem.ref_count[(idx-KERNBASE) / PGSIZE];
}
/** 初始化page_ref*/
void page_ref_init(){
    for(int i=0;i<PGROUNDUP(PHYSTOP-KERNBASE) / PGSIZE;i++) kmem.ref_count[i] = 1;
}

void
kinit() {
    initlock(&kmem.lock, "kmem");
    page_ref_init();
    freerange(end, (void *) PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char *) PGROUNDUP((uint64) pa_start);
    for (; p + PGSIZE <= (char *) pa_end; p += PGSIZE){
        kfree(p);
    }

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa) {
    struct run *r;

    if (((uint64) pa % PGSIZE) != 0 || (char *) pa < end || (uint64) pa >= PHYSTOP)
        panic("kfree");

    /**更新当前物理页面的引用数*/
    acquire(&kmem.lock);
    incr_page_ref_no_lock((uint64)pa,-1);
    int ref = get_page_ref((uint64)pa);
    if(ref > 0){//在更新过page_ref后，页表引用数仍然大于等于1的
        release(&kmem.lock);
        return ;
    }
    if(ref < 0){
        panic("page ref num is lower than 0\n");
    }

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    r = (struct run *) pa;
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void) {
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r){
        kmem.freelist = r->next;
        incr_page_ref_no_lock((uint64)r,1);//更新页表r的引用数
    }
    release(&kmem.lock);

    if (r){
        memset((char *) r, 5, PGSIZE); // fill with junk
    }

    return (void *) r;
}