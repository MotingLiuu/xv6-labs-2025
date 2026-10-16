#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv) {
  int pid = getpid();
  printf("pid: %d\n", pid);
  return 0;
}
