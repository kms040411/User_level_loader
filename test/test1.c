#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *env[]){
    int *m = (int *)malloc(sizeof(int));
    *m = 5;
    printf("m : %d\n", *m);
    printf("address : %p\n", m);
    printf("hello_world\n");
    free(m);
    return 0;
}