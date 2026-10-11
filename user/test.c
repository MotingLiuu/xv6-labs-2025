#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv) {
  sbrk(4096);
  return 0;
}

