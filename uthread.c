#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/auxv.h>

#include "common.h"
#include <uthread.h>

// Global Variable
static uint64_t saved_rsp;
static uint64_t saved_rbp;
static struct thread_info *thread_blocks;

void thread_schedule();

static inline void break_point(){
    return;
}

int main(int argc, char *argv[], char *env[]){
    // Reject if program path is not given
    if (argc < 2){ 
        printf("No program specified\n");
        return 0;
    }
    
    // Define an array for thread info structs
    // It should be static because it should be accessible for user threads
    // @TODO:
    thread_blocks = malloc(sizeof(struct thread_info) * (argc - 1));

    // Load Programs & Create Threads
    for(int program_index=1; program_index < argc; program_index++){
        // Open file
        FILE *f = fopen(argv[program_index], "rb");
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
        for(int i=0; i<phdr_num; i++){
            ELF_Pheader current_header = p_header[i];
            if(current_header.p_type != PT_LOAD) {
                continue;
            }

            uint64_t segment_start = current_header.p_vaddr;
            uint64_t segment_end = segment_start + current_header.p_memsz;
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
        uint64_t mapped_entry = entry;

        // Build stack
        #define STACK_TYPE int64_t
        #define STACK_SIZE (sizeof(STACK_TYPE))

        // Allocate a page for stack
        #define STACK_PAGE_NUM 4
        void *stack = mmap(0,
                            STACK_PAGE_NUM * sysconf(_SC_PAGE_SIZE),
                            PROT_READ | PROT_WRITE, 
                            MAP_GROWSDOWN | MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK,
                            -1,
                            0);
        memset(stack, 0, STACK_PAGE_NUM * sysconf(_SC_PAGE_SIZE));
        void *stack_pointer = stack;

        stack_pointer = stack_pointer + STACK_PAGE_NUM * sysconf(_SC_PAGE_SIZE);
        // argv data
        void **argv_pointers = (void **) malloc(sizeof(void *));
        int string_length = strlen(argv[program_index]) + 1;        // includes NULL character
        stack_pointer = stack_pointer - 16 * (int)(string_length / 16.0) - 16;
        argv_pointers[0] = stack_pointer;
        memcpy(stack_pointer, argv[program_index], string_length);

        // auxvec
        ADD_AUX(AT_NULL);
        /*ADD_AUX(AT_SYSINFO_EHDR);
        ADD_AUX(AT_HWCAP);
        ADD_AUX(AT_CLKTCK);
        ADD_AUX(AT_FLAGS);
        ADD_AUX(AT_UID);
        ADD_AUX(AT_EUID);
        ADD_AUX(AT_GID);
        ADD_AUX(AT_EGID);
        ADD_AUX(AT_HWCAP2);
        ADD_AUX(AT_BASE);*/
        /*ADD_AUX2(AT_SECURE, 0);
        ADD_AUX2(AT_ENTRY, mapped_entry);
        ADD_AUX2(AT_EXECFN, argv_pointers[0]);
        ADD_AUX2(AT_EXECFD, fileno(f));
        ADD_AUX2(AT_PHENT, e_header->e_phentsize);
        ADD_AUX2(AT_PHNUM, phdr_num);
        ADD_AUX2(AT_PHDR, p_header);*/
        ADD_AUX(AT_RANDOM);
        ADD_AUX(AT_PAGESZ);
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
        stack_pointer = stack_pointer - STACK_SIZE;
        *(STACK_TYPE *)stack_pointer = (STACK_TYPE)argv_pointers[0];

        // argc
        stack_pointer = stack_pointer - STACK_SIZE;
        *(STACK_TYPE *)stack_pointer = 1;

        free(argv_pointers);
        fclose(f);
        free(e_header);
        free(p_header);

        // Build thread_info struct
        // @TODO:

    }

    thread_schedule();

        /*
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
        break_point();

        // Save stack pointer
        asm volatile("mov %%rsp, %0": "=r"(saved_rsp));
        asm volatile("mov %%rbp, %0": "=r"(saved_rbp));

        // Set stack pointer, Goto entry point, Register at
        // https://gist.github.com/scsgxesgb/4203449
        asm volatile ("mov %0, %%rsp\n\t"
                        "jmp *%1" : : "r"(stack_pointer), "r"(mapped_entry), "d"(&&jump));

        // NOP slide
        asm volatile("nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t");
    jump:
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

        // Restore stack pointer
        asm volatile("mov %0, %%rbp": : "r"(saved_rbp));
        asm volatile("mov %0, %%rsp": : "r"(saved_rsp));

        munmap(start, address_space_size);
        munmap(stack, STACK_PAGE_NUM * sysconf(_SC_PAGE_SIZE));*/
    free(thread_blocks);
    return 0;
}

// Thread scheduler
void thread_schedule() {
    // @TODO:
}