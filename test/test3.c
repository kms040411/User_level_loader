#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[], char *env[]){
    printf("===test3.c===\n");
    char *buf = (char *)malloc(50);
    printf("THIS IS TEST3.c\n");
    char *original = "THIS IS AN ORIGINAL STRING";
    printf("original: %s\n", original);
    strcpy(buf, original);
    printf("copied: %s\n", buf);
    free(buf);
    printf("===test3.c===\n");
    return 0;
}