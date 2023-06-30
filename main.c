#include <assert.h>
#include <stdio.h>
#include "main.h"

int main(int argc, char **argv) {
    assert(argc == 3);
    FILE *ifp, *ofp;
    assert((ifp = fopen(argv[1], "r")) != NULL);
    assert((ofp = fopen(argv[2], "w")) != NULL);
    assert(fclose(ifp) == 0);
    assert(fclose(ofp) == 0);
    return 0;
}
