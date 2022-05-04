#include <stdio.h>
#include <stdlib.h>

int main() 
{

    FILE *fp = fopen("test1/hello.c", "w");
    if(fp == NULL)
    {
        /* File not created hence exit */
        printf("Unable to create file.\n");
        exit(EXIT_FAILURE);
    }
    return 0;

}
