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
} kmems[NCPU];

void kinit_per_cpu(int hart) {
    uint64 pa_start = PGROUNDUP((uint64) end);
    uint64 pa_end = (uint64) PHYSTOP;
    int per_cpu_amount = (pa_end - pa_start) / (NCPU * PGSIZE);//每个cpu拥有的页数
    uint64 cpu_begin = pa_start + per_cpu_amount * hart * PGSIZE;
    uint64 cpu_end = pa_start + per_cpu_amount * (hart + 1) * PGSIZE;
    if(cpu_end > pa_end){
        cpu_end = pa_end;
    }
    char* p = (char*)cpu_begin;
    for ( ;p + PGSIZE <= (char *)cpu_end; p += PGSIZE) {
        kfree(p);
    }
}

void
kinit() {
    for (int i = 0; i < NCPU; i++) {
        initlock(&kmems[i].lock, "kmem");
        kmems[i].freelist = 0;
        kinit_per_cpu(i);
    }
//  initlock(&kmem.lock, "kmem");
//  freerange(end, (void*)PHYSTOP);
}

//void
//freerange(void *pa_start, void *pa_end) {
//    char *p;
//    p = (char *) PGROUNDUP((uint64) pa_start);
//    for (; p + PGSIZE <= (char *) pa_end; p += PGSIZE)
//        kfree(p);
//}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa) {
    struct run *r;

    if (((uint64) pa % PGSIZE) != 0 || (char *) pa < end || (uint64) pa >= PHYSTOP)
        panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *) pa;
    push_off();
    int hart = cpuid();
    pop_off();
    acquire(&kmems[hart].lock);
    r->next = kmems[hart].freelist;
    kmems[hart].freelist = r;
    release(&kmems[hart].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void) {
    struct run *r;

    push_off();
    int hart = cpuid();
    pop_off();
    acquire(&kmems[hart].lock);
    r = kmems[hart].freelist;
    if (r){
        kmems[hart].freelist = r->next;
    }else{
        int flag = 0;
        for(int i = NCPU - 1;i >= 0;i--){
            if(i == hart) continue;
            acquire(&kmems[i].lock);
            if(kmems[i].freelist){
                r = kmems[i].freelist;
                kmems[i].freelist = r->next;
                flag = 1;
            }
            release(&kmems[i].lock);
            if(flag) break;
        }
    }

    release(&kmems[hart].lock);

    if (r)
        memset((char *) r, 5, PGSIZE); // fill with junk
    return (void *) r;
}
