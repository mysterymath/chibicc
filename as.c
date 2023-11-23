#include <cbm.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "std.h"

static Elf32_Ehdr elf = {
    {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS32, ELFDATA2LSB, EV_CURRENT,
     ELFOSABI_NONE, /*EI_ABIVERSION=*/0},
    ET_REL,
    EM_MOS,
    EV_CURRENT,
    /*e_entry=*/0,
    /*e_phoff=*/0,
    /*e_shoff=*/0,
    EF_MOS_ARCH_6502 | EF_MOS_ARCH_6502_BCD | EF_MOS_ARCH_65C02 |
        EF_MOS_ARCH_R65C02 | EF_MOS_ARCH_W65C02,
    /*e_ehsize=*/sizeof(Elf32_Ehdr),
    /*e_phentsize=*/0,
    /*e_phnum=*/0,
    /*e_shentsize=*/0,
    /*e_shnum=*/0,
    /*e_shstrndx=*/SHN_UNDEF,
};

static void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

static bool read_stmt(char *stmt, size_t max, FILE* f) {
  if (feof(f))
    return false;

  while (max--) {
    int c = fgetc(f);
    if (c == EOF)
      error("file did not end with newline");
    *stmt++ = c;
    if (c == '\n')
      return true;
  }
  error("line too long");
}

int main(void) {
  putchar(0x0f); // Enable ISO mode

  FILE *input_file = fopen("a.s", "r");

  char stmt[160];
  while (read_stmt(stmt, sizeof(stmt), input_file)) {
  }

  FILE *elf_file = fopen("a.out", "w");
  fwrite(&elf, sizeof(elf), 1, elf_file);
  fclose(elf_file);

  cbm_k_clrch();
  return 0;
}
