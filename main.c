#include <assert.h>
#include <stdio.h>
#include "main.h"

int main(int argc, char **argv) {
    assert(argc == 3);
    FILE *ifp = fopen(argv[1], "r");
    FILE *ofp = fopen(argv[2], "w");
    assert(ifp != NULL);
    assert(ofp != NULL);
    tklist_t *tkl = lexer(ifp);
    astree_t *ast = parser(tkl);
    tklist_show("[%k: '%s']", tkl);
    astree_show("[%k: '%s']", ast);
    assert(fclose(ifp) == 0);
    assert(fclose(ofp) == 0);
    tklist_free(tkl);
    astree_free(ast);
    return 0;
}
