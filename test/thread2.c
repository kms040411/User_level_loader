#include <stdio.h>
#include "../user_library.h"

int main(int argc, char *argv[], char *env[]){
    printf("THIS IS thread2.c\n");
    thread_yield();
    printf("BACK TO thread2.c\n");
    thread_yield();
    printf("thread2 exits\n");
    return 0;
}