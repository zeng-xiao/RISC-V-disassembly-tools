#include <stdio.h>
#include <stdlib.h>

#include "elf.h"

#include "dataOperate.h"

extern uint64_t shdrRiscv_attributesOff;
extern uint64_t shdrRiscv_attributesSize;

int dumpRiscv_attributes(const char *inputFileName) {
  FILE *fileHandle = fopen(inputFileName, "rb");

  fprintf(stderr, "\n\n");
  fprintf(stderr, "Contents of section .riscv.attributes:\n");

  if (!shdrRiscv_attributesOff) {
    fprintf(stderr, "\n");
    return 0;
  }

  char *strBuffer = (char *)malloc(shdrRiscv_attributesSize);

  if (!fseek(fileHandle, shdrRiscv_attributesOff, SEEK_SET))
    fread(strBuffer, 1, shdrRiscv_attributesSize, fileHandle);

  for (int charIndex = 0; charIndex < shdrRiscv_attributesSize; charIndex++)
    if (strBuffer[charIndex] == '\0')
      fprintf(stderr, " ");
    else
      fprintf(stderr, "%c", strBuffer[charIndex]);

  closeFile(fileHandle);

  fprintf(stderr, "\n\n");

  return 0;
}