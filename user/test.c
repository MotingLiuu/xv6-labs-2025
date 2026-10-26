#include "kernel/types.h"
#include "user.h"

int main(int argc, char **argv) {
  char buf[128] = {};
  int count;

  count = read(0, buf, 1);
  printf("count: %d, buf: %s\n", count, buf);

  return 0;
}
