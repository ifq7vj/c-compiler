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
    generator(ofp, ast);
    tklist_show(tkl);
    astree_show(ast);
    tklist_free(tkl);
    astree_free(ast);
    assert(fclose(ifp) == 0);
    assert(fclose(ofp) == 0);
    return 0;
}
