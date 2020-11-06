#include <stdio.h>
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

int read_elf_header(FILE*, ELF_header*);
void read_program_header(FILE*, uint64_t, uint16_t, ELF_Pheader[]);
void read_section_header(FILE*, uint64_t, uint16_t, ELF_Sheader[]);


struct addr_mapping{
    int valid;
    uint64_t absolute_offset;
    uint64_t virtual_offset;
    uint64_t size;
};
