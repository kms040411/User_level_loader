#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *env[]){
    int *m = (int *)malloc(sizeof(int));
    *m = 7;
    printf("m : %d\n", *m);
    printf("address : %p\n", m);
    printf("THIS IS TEST2.c\n");
    free(m);
    return 0;
}