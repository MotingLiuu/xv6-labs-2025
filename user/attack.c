#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
  char *p;
  int count = 0;
  while (1) {
    char *attpg = sbrk(PGSIZE);
    for (p = attpg; p < attpg + PGSIZE; p++) {
      if (*p) {
        if (strcmp("secret", p) == 0) {
          // printf("DEBUG: found secret!\n");
          p = p + strlen(p) + 1;
          count++;
        } 
        if (count == 1) {
          printf("%s\n", p);
          goto finish;
        }
      }
    }
  }

finish:
  exit(1);
}
