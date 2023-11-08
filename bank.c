#include "chibicc.h"

VoidBPtr bcalloc(size_t count, size_t size) {
  VoidBPtr ret = {1, calloc(count, size)};
  return ret;
}

void set_ram_bank(char bank) {}
