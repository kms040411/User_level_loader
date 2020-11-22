#define RUNNABLE 1

struct thread_info {
    void *sp;       // address of stack pointer
    void *bp;       // address of stack frame pointer
    void *pc;       // address of program counter
    int state;
};

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