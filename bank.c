#include "chibicc.h"

unsigned bank_ends[64];

// Simple first-fit bump allocator
VoidBPtr bmalloc(size_t size) {
  assert(size < 8192 && "Allocating more than one bank");
  for (char bank = 1; bank < 64; ++bank) {
    if (bank_ends[bank] + size > 8192)
      continue;

    VoidBPtr ptr = {bank, (char*)0xA000 + bank_ends[bank]};
    bank_ends[bank] += size;
    return ptr;
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

int bstrncmp(CharBPtr l, CharBPtr r, size_t size) {
  char tmp[size+1];
  strncpy(tmp, G(r), size);
  tmp[size] = '\0';
  return strncmp(G(l), tmp, size);
}

CharBPtr bstrndup(CharBPtr str, size_t size) {
  char tmp[size+1];
  strncpy(tmp, G(str), size);
  tmp[size] = '\0';
  VoidBPtr vdup = bcalloc(1, size+1);
  CharBPtr dup = {vdup.bank, vdup.ptr};
  strncpy(G(dup), tmp, size+1);
}
