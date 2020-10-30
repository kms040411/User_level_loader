#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <elf.h>

int main(int argc, char *argv[]){
    if (argc < 2){ 
        printf("No program specified\n");
        goto out;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL){
        printf("File not exist\n");
        goto out;
    }

    ELF_header *header = (ELF_header *) malloc(sizeof(ELF_header));
    if (header == NULL) goto file_out;

    int ret = read_elf_header(f, header);
    if (ret != 0) {
        printf("Not ELF file\n");
        goto malloc_out;
    }
    
    

malloc_out:
    free(header);

file_out:
    fclose(f);

out:
    return 0;
}