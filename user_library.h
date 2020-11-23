#include "uthread.h"
#include <sys/auxv.h>

void thread_yield(){
    struct thread_info *thread_blocks = (struct thread_info *)getauxval(100);
    if(thread_blocks == NULL) {
        // Standalone program & Do nothing
        DMSG("thread_yield: standalone\n");
    } else {
        // Loaded program
        DMSG("thread_yield: loaded\n");
        //struct thread_info *_thread_blocks = *thread_blocks;
        int *current_thread_num = (int *)getauxval(101);
        jmp_buf *loader_jmp_buf = (jmp_buf *)getauxval(102);
        struct thread_info *current_thread = &thread_blocks[*current_thread_num];
        if(!setjmp(current_thread->buf))
            longjmp(*loader_jmp_buf, 1); // Jump to loader
    }
    return;
}