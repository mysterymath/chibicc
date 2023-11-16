#include <stdio.h>
#include <stdlib.h>

void __assert(char condition, const char *str) {
  if (condition)
    return;
  printf("assertion failed: %s\n", str);
  abort();
}
