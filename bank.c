#include "chibicc.h"

unsigned bank_ends[64];

// Simple first-fit bump allocator
VoidBPtr bmalloc(size_t size) {
  assert(size < 8192 && "Allocating more than one bank");
  for (char bank = 1; bank < 64; ++bank) {
    if (bank_ends[bank] + size < 8192) {
      VoidBPtr ptr = {bank, (char*)0xA000 + bank_ends[bank]};
      bank_ends[bank] += size;
    }
  }
  printf("error: out of memory\n");
  abort();
}

VoidBPtr bcalloc(size_t count, size_t size) {
  size_t sz = count * size;
  VoidBPtr ret = bmalloc(count * size);
  memset(G(ret), 0, sz);
  return ret;
}

void set_ram_bank(char bank) {
  RAM_BANK = bank;
}
