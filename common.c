#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

#ifndef ELF_header
#define ELF_header Elf64_Ehdr
#endif

#ifndef ELF_Pheader
#define ELF_Pheader Elf64_Phdr
#endif

#ifndef ELF_Sheader
#define ELF_Sheader Elf64_Shdr
#endif

int read_elf_header(FILE *f, ELF_header *header){
    fread(header, 1, sizeof(ELF_header), f);
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
        return -ENOEXEC;    // It is not elf file
    return 0;
}

void read_program_header(FILE *f, uint64_t offset, uint16_t num, ELF_Pheader header[num]){
    uint16_t i;
    const fpos_t *_offset = (fpos_t *)&offset;
    fsetpos(f, _offset);
    for(i = 0; i < num; i++){
        fread(&header[i], 1, sizeof(ELF_Pheader), f);
    }
    return;
}

void read_section_header(FILE *f, uint64_t offset, uint16_t num, ELF_Sheader header[num]){
    uint16_t i;
    const fpos_t *_offset = (fpos_t *)&offset;
    fsetpos(f, _offset);
    for(i = 0; i < num; i++){
        fread(&header[i], 1, sizeof(ELF_Sheader), f);
    }
    return;
}