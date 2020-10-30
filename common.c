#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

#ifndef ELF_header
#define ELF_header Elf64_Ehdr
#endif

int read_elf_header(FILE *f, ELF_header *header){
    fread(header, 1, sizeof(ELF_header), f);
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
        return -ENOEXEC;    // It is not elf file
    return 0;
}