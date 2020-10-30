#include <stdio.h>
#include <elf.h>

#ifndef ELF_header
#define ELF_header Elf64_Ehdr
#endif

int read_elf_header(FILE*, ELF_header*);