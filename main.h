#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

typedef struct tklist_t tklist_t;
typedef struct astree_t astree_t;

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

typedef enum {
    AS_ADD,
    AS_SUB,
    AS_MUL,
    AS_DIV,
    AS_MOD,
    AS_NUM,
} askind_t;

struct tklist_t {
    tkkind_t kind;
    long long num;
    tklist_t *next;
};

struct astree_t {
    askind_t kind;
    long long num;
    astree_t *lhs;
    astree_t *rhs;
};

tklist_t *lexer(FILE *);
void tklist_show(const char *, tklist_t *);
void tklist_free(tklist_t *);

astree_t *parser(tklist_t *);
void astree_show(const char *, astree_t *);
void astree_free(astree_t *);

void generator(FILE *, astree_t *);

#endif
