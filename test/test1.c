#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *env[]){
    printf("===test1.c===\n");
    int *m = (int *)malloc(sizeof(int));
    *m = 5;
    printf("m : %d\n", *m);
    printf("address : %p\n", m);
    printf("hello_world\n");
    free(m);
    printf("===test1.c===\n");
    return 0;
}