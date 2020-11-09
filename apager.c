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
#include <sys/random.h>

// Get Page Boundary
#define PAGE_BOUNDARY(x) ((x) & ~(sysconf(_SC_PAGE_SIZE) - 1))

// Function Declarations
void *build_stack(int, int, char **);

static void segv_handler(int sig, siginfo_t *si, void *unused) {
    uint64_t addr = (uint64_t)si->si_addr;
    addr = addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
    void *ret = mmap((void *)addr, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if(ret == NULL){
        exit(-1);
    }
}

static inline void break_point(){
    return;
}

int main(int argc, char *argv[], char *env[]){
    // Reject if program path is not given
    if (argc < 2){ 
        printf("No program specified\n");
        return 0;
    }

    // Define signal handler
    /*
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        return 0;*/

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

    // Get the offset, size of program header
    Elf32_Off phdr_offset = e_header->e_phoff;
    uint16_t phdr_num = e_header->e_phnum;

     // Read program header
    ELF_Pheader *p_header = (ELF_Pheader *)malloc(sizeof(ELF_Pheader) * phdr_num);
    read_program_header(f, phdr_offset, phdr_num, p_header);
    
    // Get the size of address space
    uint64_t start_address = UINT64_MAX;
    uint64_t end_address = 0;
    //uint64_t start_offset = UINT64_MAX;
    //uint64_t end_offset = 0;
    for(int i=0; i<phdr_num; i++){
        ELF_Pheader current_header = p_header[i];
        if(current_header.p_type != PT_LOAD) {
            continue;
        }

        uint64_t segment_start = current_header.p_vaddr;
        uint64_t segment_end = segment_start + current_header.p_memsz;
        //uint64_t file_start = current_header.p_offset;
        //uint64_t file_end = file_start + current_header.
        if (segment_start < start_address) {
            start_address = segment_start;
        }
        if (end_address < segment_end) {
            end_address = segment_end;
        }
    }
    assert(start_address < end_address);
    uint64_t address_space_size = end_address - start_address;

    // MMAP
    // Let's think address of (void *)start as absolute address of start_address.
    void *start = mmap((void *)start_address,
                        address_space_size,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1,
                        0);
    //printf("start address: %p(0x%lx)\n", start, start_address);
    if (start == (void *)-1) {
        printf("errno: %d\n", errno);
        printf("MMAP FAILED\n");
        return 0;
    }
    
    memset(start, 0, address_space_size);

    // Map each segment
    for(int i=0; i<phdr_num; i++){
        ELF_Pheader current_header = p_header[i];
        if(current_header.p_type != PT_LOAD) {
            continue;
        }

        uint64_t segment_start_v = current_header.p_vaddr;// - start_address;
        uint64_t segment_size_v = current_header.p_memsz;
        uint64_t segment_start_f = current_header.p_offset;
        uint64_t segment_size_f = current_header.p_filesz;
        void *segment_start_p = (void *)(segment_start_v);

        // Load from the file
        const fpos_t *_offset = (fpos_t *)&segment_start_f;
        fsetpos(f, _offset);
        int ret = fread(segment_start_p,
                1,
                segment_size_f,
                f);

        // Set protection bits
        int prot = 0;
        if((current_header.p_flags & PF_W)) prot = prot | PROT_WRITE;
        if((current_header.p_flags & PF_R)) prot = prot | PROT_READ;
        if((current_header.p_flags & PF_X)) prot = prot | PROT_EXEC;
        mprotect(segment_start_p, segment_size_v, prot);
    }

    #define MAPPING(addr) ((addr) - start_address + (uint64_t)start)

    // Calc entry address
    uint64_t entry = e_header->e_entry; // Entry address
    uint64_t mapped_entry = entry;//MAPPING(entry);

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

    // NULL
    /*stack_pointer = stack_pointer - STACK_SIZE;
    *(STACK_TYPE *)stack_pointer = 0;
    stack_pointer = stack_pointer - STACK_SIZE;
    *(STACK_TYPE *)stack_pointer = 0;*/

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
