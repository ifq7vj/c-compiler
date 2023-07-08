#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

typedef struct tklist_t tklist_t;
typedef struct astree_t astree_t;
typedef struct idlist_t idlist_t;

typedef enum {
    TK_ADD,
    TK_SUB,
    TK_MUL,
    TK_DIV,
    TK_MOD,
    TK_ASG,
    TK_LPAR,
    TK_RPAR,
    TK_SCLN,
    TK_NUM,
    TK_ID,
} tkkind_t;

typedef enum {
    AS_ADD,
    AS_SUB,
    AS_MUL,
    AS_DIV,
    AS_MOD,
    AS_ASG,
    AS_NUM,
    AS_VAR,
} askind_t;

struct tklist_t {
    tkkind_t kind;
    union {
        long long num;
        char *id;
    };
    tklist_t *next;
};

struct astree_t {
    askind_t kind;
    union {
        long long num;
        size_t ofs;
    };
    astree_t *lhs, *rhs;
    astree_t *next;
};

struct idlist_t {
    char *id;
    size_t ofs;
    idlist_t *next;
};

tklist_t *lexer(FILE *);
void tklist_show(tklist_t *);
void tklist_free(tklist_t *);

astree_t *parser(tklist_t *);
void astree_show(astree_t *);
void astree_free(astree_t *);

void generator(FILE *, astree_t *);

#endif
