// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

// cow code
#define PGCOUNT ((0x88000000 - 0x80000000) / 4096)
#define REFINDEX(pa) (((uint64)pa - 0x80000000) / 4096)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  // cow code
  int refcount[PGCOUNT];
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
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int refc;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // cow code
  acquire(&kmem.lock);
  if(kmem.refcount[REFINDEX(pa)] > 0) {
    kmem.refcount[REFINDEX(pa)] -= 1;
  }
  refc = kmem.refcount[REFINDEX(pa)];
  release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  if(refc == 0) {
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
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

  /*
  if(r)
    kmem.freelist = r->next;
  */

  // cow code
  if(r) {
    kmem.freelist = r->next;
    kmem.refcount[REFINDEX(r)] = 1;
  }
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk


    kmem.rc[(uint64)r / 4096] = 1;
  }
  return (void*)r;
}

// cow code
void *
kcow(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kcow: pa's range is wrong");

  acquire(&kmem.lock);
  kmem.refcount[REFINDEX(pa)] += 1;
  release(&kmem.lock);

  return pa;
}

// cow code
int
kref(void *pa)
{
  int refc;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kcow: pa's range is wrong");

  acquire(&kmem.lock);
  refc = kmem.refcount[REFINDEX(pa)];
  release(&kmem.lock);

  return refc;
}
