#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include <stdint.h>
#include <elf.h>
#include <sys/mman.h>
#include <errno.h>

int main(int argc, char *argv[]){
    int ret;
    if (argc < 2){ 
        printf("No program specified\n");
        goto out;
    }

    FILE *f = fopen(argv[1], "rb");
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

    struct addr_mapping *mapping_table = (struct addr_mapping *)malloc(sizeof(struct addr_mapping) * phdr_num);
    if (mapping_table == NULL) goto malloc_out2;

    for(unsigned i=0; i<phdr_num; i++){
        ELF_Pheader current_header = p_header[i];
        if (current_header.p_type != PT_LOAD){
            mapping_table[i].valid = 0;
            continue;
        }

        // Load Segment
        Elf64_Off f_offset = current_header.p_offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
        Elf64_Xword f_size = current_header.p_filesz;
        Elf64_Addr adr_offset = current_header.p_vaddr;
        Elf64_Xword adr_size = current_header.p_memsz;
        Elf64_Word flag = current_header.p_flags;

        // Do mapping
        int prot = PROT_READ | PROT_EXEC;
        uint64_t load_bias = current_header.p_offset - f_offset;
        void *mapping = mmap(0, adr_size + load_bias, prot, MAP_PRIVATE, fileno(f), f_offset);
        if ((int64_t)mapping == -1){
            printf("mapping failed\n");
            exit(-1);
        }
        mapping_table[i].valid = 1;
        mapping_table[i].absolute_offset = adr_offset - load_bias;
        mapping_table[i].virtual_offset = mapping;
        mapping_table[i].size = adr_size + load_bias;
    }

    /*
    // Get the offset, size of section header
    Elf64_Off shdr_offset = e_header->e_shoff;
    uint16_t shdr_num = e_header->e_shnum;

    // Read section header
    ELF_Sheader *s_header = (ELF_Sheader *)malloc(sizeof(ELF_Sheader) * shdr_num);
    if (s_header == NULL) goto malloc_out1;
    read_program_header(f, shdr_offset, shdr_num, s_header);

    for(unsigned i=0; i<shdr_num; i++){
        ELF_Sheader current_header = s_header[i];
        // Load Section
        Elf64_Off f_offset = current_header.sh_offset & ~(sysconf(_SC_PAGE_SIZE) - 1); // Align to page boundary
        Elf64_Xword size = current_header.sh_size;
        Elf64_Addr adr_offset = current_header.sh_addr;
        Elf64_Word flag = current_header.sh_flags;
        // Do mapping
        uint64_t load_bias = current_header.sh_offset - f_offset;
        int prot = PROT_READ | PROT_EXEC;
        printf("%ld, %ld\n", size, f_offset);
        void *mapping = mmap(0, size, prot, MAP_PRIVATE, fileno(f), f_offset);
        if ((int64_t)mapping == -1){
            printf("mapping failed\n");
            printf("%d\n\n", errno);
            continue;
        }
        printf("%p\n", mapping);
    }*/

    Elf64_Addr entry = e_header->e_entry; // Entry address
    uint64_t mapped_entry = 0;

    for(unsigned i=0; i<phdr_num; i++){
        if(mapping_table[i].valid){
            if(mapping_table[i].absolute_offset < entry < mapping_table[i].absolute_offset + mapping_table[i].size){
                mapped_entry = entry + (mapping_table[i].virtual_offset - mapping_table[i].absolute_offset);
                break;
            }
        }
    }

    // Build Stack

    // Clean up registers
    __asm__ __volatile__ ("xor %rax, %rax\n\t"
                            "xor %rbx, %rbx\n\t"
                            "xor %rcx, %rcx\n\t"
                            "xor %rdx, %rdx\n\t"
                            "xor %rsi, %rsi\n\t"
                            "xor %rdi, %rdi\n\t"
                            "xor %r8, %r8\n\t"
                            "xor %r9, %r9\n\t"
                            "xor %r10, %r10\n\t"
                            "xor %r11, %r11\n\t"
                            "xor %r12, %r12\n\t"
                            "xor %r13, %r13\n\t"
                            "xor %r14, %r14\n\t"
                            "xor %r15, %r15\n\t");

    printf("0x%lx\n", mapped_entry);
    // Goto entry point
    __asm__ __volatile__ ("jmp %0": : "A"(mapped_entry));

malloc_out3:
    free(mapping_table);

malloc_out2:
    free(p_header);

malloc_out1:
    free(e_header);

file_out:
    fclose(f);

out:
    return 0;
}