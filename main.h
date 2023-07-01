#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

typedef struct tklist_t tklist_t;

typedef enum {
    TK_ADD,
    TK_SUB,
    TK_MUL,
    TK_DIV,
    TK_MOD,
    TK_LPAR,
    TK_RPAR,
    TK_NUM,
} tkkind_t;

struct tklist_t {
    tkkind_t kind;
    long long num;
    tklist_t *next;
};

tklist_t *lexer(FILE *);
void tklist_show(const char *, tklist_t *);
void tklist_free(tklist_t *);

#endif
