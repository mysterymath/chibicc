#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;
typedef uint16_t Elf32_Versym;
typedef unsigned char Elf_Byte;
typedef uint16_t Elf32_Half;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf32_Xword;

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NINDENT 16

typedef struct {
  unsigned char e_ident[EI_NINDENT];
  __attribute__((aligned(2))) uint16_t e_type;
  __attribute__((aligned(2))) uint16_t e_machine;
  __attribute__((aligned(4))) uint32_t e_version;
  __attribute__((aligned(4))) Elf32_Addr e_entry;
  __attribute__((aligned(4))) Elf32_Off e_phoff;
  __attribute__((aligned(4))) Elf32_Off e_shoff;
  __attribute__((aligned(4))) uint32_t e_flags;
  __attribute__((aligned(2))) uint16_t e_ehsize;
  __attribute__((aligned(2))) uint16_t e_phentsize;
  __attribute__((aligned(2))) uint16_t e_phnum;
  __attribute__((aligned(2))) uint16_t e_shentsize;
  __attribute__((aligned(2))) uint16_t e_shnum;
  __attribute__((aligned(2))) uint16_t e_shstrndx;
} Elf32_Ehdr;

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASSNONE 0
#define ELFCLASS32 1

#define ELFDATANONE 0
#define ELFDATA2LSB 1

#define EV_NONE 0
#define EV_CURRENT 1

#define ELFOSABI_NONE 0
#define ELFOSABI_SYSV 1

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define EM_NONE 0
#define EM_MOS 6502

#define EF_MOS_ARCH_6502 0x00000001 // Core NMOS 6502 instruction set, no BCD
#define EF_MOS_ARCH_6502_BCD 0x00000002 // BCD support, including CLD and SED
#define EF_MOS_ARCH_6502X 0x00000004    // "Illegal" NMOS 6502 instructions
#define EF_MOS_ARCH_65C02 0x00000008    // Core 65C02 instruction set
#define EF_MOS_ARCH_R65C02 0x00000010   // Rockwell extensions to 65C02 insns
#define EF_MOS_ARCH_W65C02 0x00000020   // WDC extensions to 65C02 insns
#define EF_MOS_ARCH_W65816 0x00000100   // 65816 instructions
#define EF_MOS_ARCH_65EL02 0x00000200   // 65EL02 instructions
#define EF_MOS_ARCH_65CE02 0x00000400   // 65CE02 instructions
#define EF_MOS_ARCH_HUC6280 0x00000800  // HuC6280 instructions
#define EF_MOS_ARCH_65DTV02 0x00001000  // C64DTV 6502 instructions
#define EF_MOS_ARCH_4510 0x00002000     // CSG 4510 instructions
#define EF_MOS_ARCH_45GS02 0x0004000    // 45GS02 instructions
#define EF_MOS_ARCH_SWEET16 0x00010000  // SWEET16 instructions
#define EF_MOS_ARCH_SPC700 0x00020000   // SPC700 instructions,

#define SHN_UNDEF 0

#endif // not _ELF_H
