#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    char* p = argv[1];

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("\nmain:\n");

    printf("    mov rax, %ld\n", strtol(p, &p, 10));
    printf("    ret\n");

    return 0;
}
