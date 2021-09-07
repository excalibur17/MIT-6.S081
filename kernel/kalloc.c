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

// 128*1024*1024/4096 = 2^15
int ref_count[(PHYSTOP-KERNBASE)/PGSIZE]; // do not use uint
struct spinlock ref_count_lock;
#define REF_IDX(x) (((uint64)(x)-KERNBASE)/PGSIZE)

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref_count[REF_IDX(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  int tmp = 0;
  acquire(&ref_count_lock);
  if((uint64)pa >= KERNBASE && (uint64)pa < PHYSTOP){
    if(ref_count[REF_IDX(pa)] < 1)
      panic("kfree ref");
    tmp = --ref_count[REF_IDX(pa)];
  }
  release(&ref_count_lock);
  if(tmp > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
     // must have this
    if((uint64)r >= KERNBASE && (uint64)r < PHYSTOP){
      acquire(&ref_count_lock);
      if(ref_count[REF_IDX(r)] != 0)
        panic("kalloc: ref_count");
      ref_count[REF_IDX(r)] = 1;
      release(&ref_count_lock);
    }
  }
    
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int get_refcount(uint64 pa){
  // acquire(&ref_count_lock);
  int tmp = ref_count[(pa-KERNBASE)/PGSIZE];
  // release(&ref_count_lock);
  return tmp;
}

// must not be uint x ...
void update_refcount(uint64 pa, int x){
  acquire(&ref_count_lock);
  ref_count[(pa-KERNBASE)/PGSIZE] += x;
  release(&ref_count_lock);
}