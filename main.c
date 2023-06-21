#include <stdlib.h>
#include <stdio.h>

#include "main.h"

char *code;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    code = argv[1];
    tokenize();
    prog();
    gen_code();
    return 0;
}
