#include <stdio.h>
#include <setjmp.h>

#define TERMINATED 0
#define RUNNABLE 1

struct thread_info {
    void *sp;       // address of stack pointer
    void *pc;       // address of program counter
    int state;
    jmp_buf buf;

    int initialized;

    void *stack_mem;
    int stack_size;
    void *program_mem;
    int program_size;
};

static struct thread_info *thread_blocks;
static jmp_buf loader_jmp_buf;
static int current_thread_num = 0;
static int next_thread_num = 0;

void thread_yield(){
    if(thread_blocks == NULL) {
        // Standalone program & Do nothing
        printf("standalone\n");
    } else {
        // Loaded program
        struct thread_info *current_thread = &thread_blocks[current_thread_num];
        if(!setjmp(current_thread->buf))
            longjmp(loader_jmp_buf, 1); // Jump to loader
    }
    return;
}

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

#define __UNREACHABLE__             \
do {                                \
    printf("UNREACHABLE\n");        \
    exit(-1);                       \
} while(0)
