#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include <stdint.h>
#include <elf.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/auxv.h>

// Get Page Boundary
#define PAGE_SIZE        sysconf(_SC_PAGE_SIZE)
#define PAGE_BOUNDARY(x) ((x) & ~(sysconf(_SC_PAGE_SIZE) - 1))

// Function Declarations
void *build_stack(int, int, char **);
static void break_point();

// Global variables
static struct addr_mapping *mapping_table;
static uint64_t table_entry_num;

/*
static void segv_handler(int sig, siginfo_t *si, void *unused) {
    void *ret;
    uint64_t addr = (uint64_t)si->si_addr;
    printf("fault address: 0x%lx\n", addr);

    // Find mapping table entry
    struct addr_mapping *found = NULL;
    for (int i=0; i<table_entry_num; i++){
        struct addr_mapping current_entry = mapping_table[i];
        if (current_entry.valid == 1) {
            if (current_entry.address_start <= addr && addr <= (current_entry.address_start + current_entry.address_size)){
                found = &mapping_table[i];
                break;
            }
        }
    }
    if (!found) {
        printf("mapping table not found\n");
        exit(-1);
        return;     // Only for debug
    }

    // mmap
    uint64_t address_start = PAGE_BOUNDARY(addr);
    uint64_t bias = addr - address_start;
    ret = mmap((void *)address_start, PAGE_SIZE, found->prot, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if(ret == (void *)-1){
        printf("Mapping failed\n");
        exit(-1);
    }

    // Load from file
    


    break_point();
}*/


static void segv_handler(int sig, siginfo_t *si, void *unused) {
    void *ret;
    uint64_t addr = (uint64_t)si->si_addr;
    //printf("fault address: 0x%lx\n", addr);

    uint64_t aligned_start = PAGE_BOUNDARY(addr);
    uint64_t aligned_end = aligned_start + PAGE_SIZE;
    
    //printf("aligned_start: 0x%lx, aligned_end: 0x%lx\n", aligned_start, aligned_end);
    // MMAP
    ret = mmap((void *)aligned_start, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if(ret == (void *)-1){
        printf("Mapping failed\n");
        exit(-1);
    }

    // Find list of sections
    int counter = 0;
    for (int i=0; i<table_entry_num; i++){
        struct addr_mapping current_entry = mapping_table[i];
        uint64_t address_start = current_entry.address;
        uint64_t address_end = current_entry.address + current_entry.size;
        //printf("================\n");
        //printf("address_start: 0x%lx, address_end, 0x%lx\n", address_start, address_end);
        if((aligned_start <= address_start && address_start < aligned_end) ||
            (aligned_start <= address_end && address_start < aligned_end)){
            // This section resides on this page
            counter++;
            uint64_t load_start = (aligned_start > address_start) ? aligned_start : address_start;
            uint64_t load_end = (aligned_end < address_end) ? aligned_end : address_end;
            uint64_t load_size = load_end - load_start;
            //printf("entry_start: 0x%lx, entry_end: 0x%lx ", address_start, address_end);
            //printf("load_start: 0x%lx, load_end: 0x%lx ", load_start, load_end);
            if (current_entry.load) {
                // FILE BACKED
                //printf("File Backed\n");
                uint64_t file_start = load_start - current_entry.address + current_entry.file_offset;
                const fpos_t *_file_start = (fpos_t *)&file_start;
                fsetpos(current_entry.f, _file_start);
                fread((void *)load_start, 1, load_size, current_entry.f);
            } else {
                // NOT FILE BACKED
                //printf("Not File Backed\n");
                memset((void *) load_start, 0, load_size);
            }
        }
    }
    if (counter == 0) {
        printf("Invalid Pointer\n");
        exit(-1);
    }

    break_point();
}

static void break_point(){
    return;
}

int main(int argc, char *argv[], char *env[]){
    // Reject if program path is not given
    if (argc < 2){ 
        printf("No program specified\n");
        return 0;
    }

    // Define signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        return 0;

    // Open file
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL){
        printf("File not exist\n");
        return 0;
    }

    // Build ELF_header
    ELF_header *e_header = (ELF_header *) malloc(sizeof(ELF_header));
    
    // Read ELF header
    int ret = read_elf_header(f, e_header);
    if (ret != 0) {
        printf("Not ELF file\n");
        return 0;
    }

    // Get the offset, size of program header & section header
    Elf64_Off phdr_offset = e_header->e_phoff;
    uint16_t phdr_num = e_header->e_phnum;
    Elf64_Off shdr_offset = e_header->e_shoff;
    uint64_t shdr_num = e_header->e_shnum;


     // Read program header
    ELF_Pheader *p_header = (ELF_Pheader *)malloc(sizeof(ELF_Pheader) * phdr_num);
    read_program_header(f, phdr_offset, phdr_num, p_header);

    // Read section header
    ELF_Sheader *s_header = (ELF_Sheader *)malloc(sizeof(ELF_Sheader) * shdr_num);
    read_section_header(f, shdr_offset, shdr_num, s_header);

    mapping_table = (struct addr_mapping *) malloc(sizeof(struct addr_mapping) * shdr_num);
    table_entry_num = shdr_num;
    // Get the size of address space
    for(int i=0; i<shdr_num; i++){
    //for(int i=0; i<phdr_num; i++){
        
        ELF_Sheader current_header = s_header[i];
        if(current_header.sh_type == SHT_NOBITS){
            mapping_table[i].load = 0;
        } else {
            mapping_table[i].load = 1;
        }


        mapping_table[i].address = current_header.sh_addr;
        mapping_table[i].file_offset = current_header.sh_offset;
        mapping_table[i].size = current_header.sh_size;
        mapping_table[i].flag = current_header.sh_flags;
        mapping_table[i].align = current_header.sh_addralign;
        mapping_table[i].f = f;
        /*
        if(current_header.p_type != PT_LOAD) {
            mapping_table[i].valid = 0;
            continue;
        }

        uint64_t segment_start = current_header.p_vaddr;
        uint64_t file_start = current_header.p_offset;
        
        mapping_table[i].valid = 1;
        mapping_table[i].address_start = segment_start;
        mapping_table[i].address_size = current_header.p_memsz;
        mapping_table[i].file_start = file_start;
        mapping_table[i].file_size = current_header.p_filesz;
        mapping_table[i].fd = fileno(f);

        int prot = 0;
        if((current_header.p_flags & PF_W)) prot = prot | PROT_WRITE;
        if((current_header.p_flags & PF_R)) prot = prot | PROT_READ;
        if((current_header.p_flags & PF_X)) prot = prot | PROT_EXEC;
        mapping_table[i].prot = prot;*/
    }

    // Calc entry address
    uint64_t mapped_entry = e_header->e_entry;

    // Build stack
    #define STACK_TYPE int64_t
    #define STACK_SIZE (sizeof(STACK_TYPE))

    // Allocate a page for stack
    /*void *stack = mmap(0,
                        2 * sysconf(_SC_PAGE_SIZE),
                        PROT_READ | PROT_WRITE, 
                        MAP_GROWSDOWN | MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK,
                        -1,
                        0);*/
    void *stack = malloc(2 * sysconf(_SC_PAGE_SIZE));
    memset(stack, 0, sysconf(_SC_PAGE_SIZE));
    void *stack_pointer = stack;

    // argv data
    int argument_num = argc - 1;
    stack_pointer = stack_pointer + 2 * sysconf(_SC_PAGE_SIZE);
    void **argv_pointers = (void **) malloc(sizeof(void *) * argument_num);
    for (int i=argument_num; i>=1; i--){
        int string_length = strlen(argv[i]) + 1;        // includes NULL character
        stack_pointer = stack_pointer - 16 * (int)(string_length / 16) - 16;
        argv_pointers[i] = stack_pointer;
        memcpy(stack_pointer, argv[i], string_length);
    }

    // auxvec
    #define ADD_AUX(id)                                             \
	do {                                                            \
        stack_pointer = stack_pointer - STACK_SIZE;                 \
		*(STACK_TYPE *)stack_pointer = (STACK_TYPE)getauxval(id);   \
        stack_pointer = stack_pointer - STACK_SIZE;                 \
		*(STACK_TYPE *)stack_pointer = (STACK_TYPE)id;              \
	} while (0)

    #define ADD_AUX2(id, val)                                   \
	do {                                                        \
        stack_pointer = stack_pointer - STACK_SIZE;             \
		*(STACK_TYPE *)stack_pointer = (STACK_TYPE)val;         \
        stack_pointer = stack_pointer - STACK_SIZE;             \
		*(STACK_TYPE *)stack_pointer = (STACK_TYPE)id;          \
	} while (0)

    ADD_AUX(AT_NULL);
    /*ADD_AUX(AT_SYSINFO_EHDR);
    ADD_AUX(AT_HWCAP);
    ADD_AUX(AT_PAGESZ);
    ADD_AUX(AT_CLKTCK);
    ADD_AUX(AT_FLAGS);
    ADD_AUX(AT_UID);
    ADD_AUX(AT_EUID);
    ADD_AUX(AT_GID);
    ADD_AUX(AT_EGID);
    ADD_AUX(AT_HWCAP2);
    ADD_AUX(AT_BASE);*/
    ADD_AUX2(AT_SECURE, 0);
    ADD_AUX2(AT_ENTRY, mapped_entry);
    ADD_AUX2(AT_EXECFN, argv_pointers[0]);
    ADD_AUX2(AT_EXECFD, fileno(f));
    ADD_AUX2(AT_PHENT, e_header->e_phentsize);
    ADD_AUX2(AT_PHNUM, phdr_num);
    ADD_AUX2(AT_PHDR, p_header);
    ADD_AUX(AT_RANDOM);
    ADD_AUX(AT_PLATFORM);

    // NULL
    stack_pointer = stack_pointer - STACK_SIZE;
    *(STACK_TYPE *)stack_pointer = 0;

    // env pointers
    while( *env != NULL ){
        stack_pointer = stack_pointer - STACK_SIZE;
        *(STACK_TYPE *)stack_pointer = (STACK_TYPE)(*env);
        env++;
    }
  
    // NULL
    stack_pointer = stack_pointer - STACK_SIZE;
    *(STACK_TYPE *)stack_pointer = 0;

    // argv pointers
    for (int i=argument_num; i>=1; i--){
        stack_pointer = stack_pointer - STACK_SIZE;
        *(STACK_TYPE *)stack_pointer = (STACK_TYPE)argv_pointers[i];
    }

    // argc
    stack_pointer = stack_pointer - STACK_SIZE;
    *(STACK_TYPE *)stack_pointer = argument_num;

    break_point();

    //free(argv_pointers);

    //printf("stack: %p, entry: 0x%lx\n", stack_pointer, mapped_entry);

    //void *temp = stack_pointer;
    // Show Stack
    /*for (int i=0; i<46; i++){
        printf("%lx\n", *(uint64_t *)temp);
        temp = temp + STACK_SIZE;
    }*/

    //fclose(f);
    //free(e_header);
    //free(p_header);

    // Clean up registers
    asm volatile ("xor %rax, %rax\n\t"
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
                "xor %r15, %r15");

    // Set stack pointer, Goto entry point
    asm volatile ("mov %0, %%rsp\n\t"
                    "jmp *%1" : : "r"(stack_pointer), "r"(mapped_entry), "d"(0));

    return 0;
}
