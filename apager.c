#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <elf.h>

int main(int argc, char *argv[]){
    int ret;
    if (argc < 2){ 
        printf("No program specified\n");
        goto out;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL){
        printf("File not exist\n");
        goto out;
    }

    ELF_header *e_header = (ELF_header *) malloc(sizeof(ELF_header));
    if (e_header == NULL) goto file_out;

    // Read ELF header
    ret = read_elf_header(f, e_header);
    if (ret != 0) {
        printf("Not ELF file\n");
        goto malloc_out1;
    }
    
    // Get the offset, size of program header
    Elf64_Off phdr_offset = e_header->e_phoff;
    uint16_t phdr_num = e_header->e_phnum;

    // Read program header
    ELF_Pheader *p_header = (ELF_Pheader *)malloc(sizeof(ELF_Pheader) * phdr_num);
    if (p_header == NULL) goto malloc_out1;
    read_program_header(f, phdr_offset, phdr_num, p_header);

    

malloc_out2:
    free(p_header);

malloc_out1:
    free(e_header);

file_out:
    fclose(f);

out:
    return 0;
}