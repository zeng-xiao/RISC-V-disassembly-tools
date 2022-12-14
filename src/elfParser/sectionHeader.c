#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"

#include "dataOperate.h"
#include "sectionHeader.h"

extern uint64_t shdrAddress;
extern uint16_t shdrNumber;
extern uint16_t shdrSize;
extern uint16_t shdrStrtabIndex;

uint64_t shdrDebug_strOff = 0;
uint64_t shdrDebug_strSize = 0;

uint64_t shdrCommentOff = 0;
uint64_t shdrCommentSize = 0;

uint64_t shdrRiscv_attributesOff = 0;
uint64_t shdrRiscv_attributesSize = 0;

uint64_t shdrGCCcommandlineOff = 0;
uint64_t shdrGCCcommandlineSize = 0;

uint64_t shdrTextOff = 0;
uint64_t shdrTextSize = 0;

static void printf_elf64_header(Elf64_Info_Shdr *i_shdr) {
  fprintf(stderr, "\n\n");

  fprintf(stderr, "Section Headers Info:\n");
  fprintf(stderr,
          "  [Nr]    Name                     SectionType       Address   "
          "       SectionOffsetInFile SectionSize "
          "EntrySize Flags "
          "LinkToSection Info "
          "Alignment\n");

  for (int shdrIndex = 0; shdrIndex < shdrNumber; shdrIndex++) {
    fprintf(
        stderr,
        "  [%02d]    %-19s      %-16s  %016lx "
        "%06lx              %06lx      %03lx       %s    %02d            %02d  "
        "     %lx\n",
        shdrIndex, i_shdr[shdrIndex].i_sh_name, i_shdr[shdrIndex].i_sh_type,
        i_shdr[shdrIndex].i_sh_addr, i_shdr[shdrIndex].i_sh_offset,
        i_shdr[shdrIndex].i_sh_size, i_shdr[shdrIndex].i_sh_entsize,
        i_shdr[shdrIndex].i_sh_flags, i_shdr[shdrIndex].i_sh_link,
        i_shdr[shdrIndex].i_sh_info, i_shdr[shdrIndex].i_sh_addralign);
  }

  fprintf(stderr, "\n\n");
}

static void sh_name(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_name = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_name, sizeof(shdr[shdrIndex].sh_name));
}

static bool sh_name_str(int shdrIndex, Elf64_Shdr *shdr,
                        Elf64_Info_Shdr *i_shdr, FILE *fileHandle) {
  char *strBuffer = malloc(1024);

  if (!strBuffer) {
    printf("memory allocation failed for strBuffer\n");
    return false;
  }

  uint64_t strOffset;

  strOffset = shdr[shdrStrtabIndex].sh_offset + shdr[shdrIndex].sh_name;
  if (!fseek(fileHandle, strOffset, SEEK_SET))
    fscanf(fileHandle, "%s", strBuffer);
  i_shdr[shdrIndex].i_sh_name = strBuffer;

  if (!strcmp(".debug_str", strBuffer)) {
    shdrDebug_strOff = shdr[shdrIndex].sh_offset;
    shdrDebug_strSize = shdr[shdrIndex].sh_size;
  }

  if (!strcmp(".comment", strBuffer)) {
    shdrCommentOff = shdr[shdrIndex].sh_offset;
    shdrCommentSize = shdr[shdrIndex].sh_size;
  }

  if (!strcmp(".riscv.attributes", strBuffer)) {
    shdrRiscv_attributesOff = shdr[shdrIndex].sh_offset;
    shdrRiscv_attributesSize = shdr[shdrIndex].sh_size;
  }

  if (!strcmp(".GCC.command.line", strBuffer)) {
    shdrGCCcommandlineOff = shdr[shdrIndex].sh_offset;
    shdrGCCcommandlineSize = shdr[shdrIndex].sh_size;
  }

  if (!strcmp(".text", strBuffer)) {
    shdrTextOff = shdr[shdrIndex].sh_offset;
    shdrTextSize = shdr[shdrIndex].sh_size;
  }
}

static void sh_type(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_type = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_type, sizeof(shdr[shdrIndex].sh_type));

  switch (shdr[shdrIndex].sh_type) {
  case SHT_NULL:
    i_shdr[shdrIndex].i_sh_type = "NULL";
    break;
  case SHT_DYNSYM:
    i_shdr[shdrIndex].i_sh_type = "DYNSYM";
    break;

  case SHT_STRTAB:
    i_shdr[shdrIndex].i_sh_type = "STRTAB";
    break;

  case SHT_SYMTAB_SHNDX:
    i_shdr[shdrIndex].i_sh_type = "SYMTAB_SHNDX";
    break;

  case SHT_SYMTAB:
    i_shdr[shdrIndex].i_sh_type = "SYMTAB";
    break;

  case SHT_GROUP:
    i_shdr[shdrIndex].i_sh_type = "GROUP";
    break;

  case SHT_REL:
    i_shdr[shdrIndex].i_sh_type = "REL";
    break;

  case SHT_RELA:
    i_shdr[shdrIndex].i_sh_type = "RELA";
    break;

  case SHT_RELR:
    i_shdr[shdrIndex].i_sh_type = "RELR";
    break;

    /* Having a zero sized section is not illegal according to the ELF
     * standard, but it might be an indication that something is wrong.  So
     * issue a warning if we are running in lint mode.  */
  case SHT_NOTE:
    i_shdr[shdrIndex].i_sh_type = "NOTE";
    break;
  case SHT_NOBITS:
    i_shdr[shdrIndex].i_sh_type = "NOBITS";
    break;
  case SHT_PROGBITS:
    i_shdr[shdrIndex].i_sh_type = "PROGBITS";
    break;
  case SHT_RISCV_ATTRIBUTES:
    i_shdr[shdrIndex].i_sh_type = "RISCV_ATTRIBUTES";
  default:
    break;
  }

  if (!i_shdr[shdrIndex].i_sh_type)
    i_shdr[shdrIndex].i_sh_type = "unknown";
}

static void sh_flags(int shdrIndex, Elf64_Shdr *shdr,
                     Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  static char flagBuffer[1024];
  char *p = flagBuffer;
  shdr[shdrIndex].sh_flags = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_flags, sizeof(shdr[shdrIndex].sh_flags));

  while (shdr[shdrIndex].sh_flags) {
    uint64_t flag;

    flag = shdr[shdrIndex].sh_flags & -shdr[shdrIndex].sh_flags;
    shdr[shdrIndex].sh_flags &= ~flag;
    switch (flag) {
    case SHF_WRITE:
      *p = 'W';
      break;
    case SHF_ALLOC:
      *p = 'A';
      break;
    case SHF_EXECINSTR:
      *p = 'X';
      break;
    case SHF_MERGE:
      *p = 'M';
      break;
    case SHF_STRINGS:
      *p = 'S';
      break;
    case SHF_INFO_LINK:
      *p = 'I';
      break;
    case SHF_LINK_ORDER:
      *p = 'L';
      break;
    case SHF_OS_NONCONFORMING:
      *p = 'O';
      break;
    case SHF_GROUP:
      *p = 'G';
      break;
    case SHF_TLS:
      *p = 'T';
      break;
    case SHF_EXCLUDE:
      *p = 'E';
      break;
    case SHF_COMPRESSED:
      *p = 'C';
      break;
    default:
      break;
    }
    p++;
  }
  i_shdr[shdrIndex].i_sh_flags = flagBuffer;
}

static void sh_addr(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_addr = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_addr, sizeof(shdr[shdrIndex].sh_addr));
  i_shdr[shdrIndex].i_sh_addr = shdr[shdrIndex].sh_addr;
}

static void sh_offset(int shdrIndex, Elf64_Shdr *shdr,
                      Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_offset = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_offset, sizeof(shdr[shdrIndex].sh_offset));
  i_shdr[shdrIndex].i_sh_offset = shdr[shdrIndex].sh_offset;
}

static void sh_size(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_size = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_size, sizeof(shdr[shdrIndex].sh_size));
  i_shdr[shdrIndex].i_sh_size = shdr[shdrIndex].sh_size;
}

static void sh_link(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_link = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_link, sizeof(shdr[shdrIndex].sh_link));
  i_shdr[shdrIndex].i_sh_link = shdr[shdrIndex].sh_link;
}

static void sh_info(int shdrIndex, Elf64_Shdr *shdr,
                    Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_info = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_info, sizeof(shdr[shdrIndex].sh_info));
  i_shdr[shdrIndex].i_sh_info = shdr[shdrIndex].sh_info;
}

static void sh_addralign(int shdrIndex, Elf64_Shdr *shdr,
                         Elf64_Auxiliary_Shdr *a_shdr,
                         Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_addralign = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_addralign, sizeof(shdr[shdrIndex].sh_addralign));
  i_shdr[shdrIndex].i_sh_addralign = shdr[shdrIndex].sh_addralign;
}

static void sh_entsize(int shdrIndex, Elf64_Shdr *shdr,
                       Elf64_Auxiliary_Shdr *a_shdr, Elf64_Info_Shdr *i_shdr) {
  shdr[shdrIndex].sh_entsize = byte_get_little_endian(
      a_shdr[shdrIndex].a_sh_entsize, sizeof(shdr[shdrIndex].sh_entsize));
  i_shdr[shdrIndex].i_sh_entsize = shdr[shdrIndex].sh_entsize;
}

int process_section_header(const char *inputFileName) {
  FILE *fileHandle = fopen(inputFileName, "rb");

  Elf64_Shdr shdr[shdrNumber];
  Elf64_Info_Shdr i_shdr[shdrNumber];
  Elf64_Auxiliary_Shdr a_shdr[shdrNumber];

  if (!fseek(fileHandle, shdrAddress, SEEK_SET))
    for (int shdrIndex = 0; shdrIndex < shdrNumber; shdrIndex++) {
      fread(&a_shdr[shdrIndex], 1, shdrSize, fileHandle);

      sh_name(shdrIndex, shdr, a_shdr, i_shdr);
      sh_type(shdrIndex, shdr, a_shdr, i_shdr);
      sh_flags(shdrIndex, shdr, a_shdr, i_shdr);
      sh_addr(shdrIndex, shdr, a_shdr, i_shdr);
      sh_offset(shdrIndex, shdr, a_shdr, i_shdr);
      sh_size(shdrIndex, shdr, a_shdr, i_shdr);
      sh_link(shdrIndex, shdr, a_shdr, i_shdr);
      sh_info(shdrIndex, shdr, a_shdr, i_shdr);
      sh_addralign(shdrIndex, shdr, a_shdr, i_shdr);
      sh_entsize(shdrIndex, shdr, a_shdr, i_shdr);
    }

  for (int shdrIndex = 0; shdrIndex < shdrNumber; shdrIndex++)
    sh_name_str(shdrIndex, shdr, i_shdr, fileHandle);

  close_file(fileHandle);

  printf_elf64_header(i_shdr);

  return 0;
}
