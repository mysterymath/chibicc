#include <cbm.h>
#include <elf.h>

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

int main(void) {
  cbm_k_setnam("@:a.out,S,W");
  cbm_k_setlfs(1, 8, 2);
  cbm_k_open();
  cbm_k_chkout(1);

  char *c = (char *)&elf;
  for (unsigned i = 0; i < sizeof(elf); ++i)
    cbm_k_chrout(*c++);

  cbm_k_close(1);
  cbm_k_clrch();

  return 0;
}
