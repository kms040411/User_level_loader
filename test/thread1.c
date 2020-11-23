#include <stdio.h>
#include "../uthread.h"

int main(int argc, char *argv[], char *env[]){
    printf("THIS IS thread1.c\n");
    thread_yield();
    printf("BACK TO thread1.c\n");
    thread_yield();
    printf("thread1 exits\n");
    return 0;
}