#include "kernel/types.h"
#include "user/user.h"
#include "kernel/riscv.h"

void
print_pte(uint64 va)
{
    pte_t pte = (pte_t) pgpte((void *) va);
    printf("va 0x%lx pte 0x%lx pa 0x%lx perm 0x%lx\n", va, pte, PTE2PA(pte), PTE_FLAGS(pte));
}

void
print_pgtbl()
{
  printf("print_pgtbl starting\n");
  for (uint64 i = 0; i < 10; i++) {
    print_pte(i * PGSIZE);
  }
  uint64 top = MAXVA/PGSIZE;
  for (uint64 i = top-10; i < top; i++) {
    print_pte(i * PGSIZE);
  }
  printf("print_pgtbl: OK\n");
}

int main(int argc, char** argv) {
  char *end;
  uint64 st_spva, ed_spva;

  end = sbrk(0);
  printf("end before sbrk: %p\n", (char *)end);

  sbrk(2 * SUPERPGSIZE);

  end = sbrk(0);
  printf("end after sbrk: %p\n", (char *)end);

  st_spva = SUPERPGROUNDDOWN((uint64)end - SUPERPGSIZE);
  ed_spva = SUPERPGROUNDUP((uint64)end - SUPERPGSIZE);
  print_pte(st_spva);
  print_pte(st_spva + PGSIZE);

  sbrk(-SUPERPGSIZE);
  printf("st_spva: %p, ed_spva: %p\n", (char *)st_spva, (char *)ed_spva);

  print_pte(st_spva);
  print_pte(st_spva + PGSIZE);

  return 0;
}

