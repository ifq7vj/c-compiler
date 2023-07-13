#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
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
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_LE,
    TK_GT,
    TK_GE,
    TK_ASG,
    TK_LPRN,
    TK_RPRN,
    TK_LBRC,
    TK_RBRC,
    TK_SCLN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_RET,
    TK_ID,
    TK_NUM,
} tkkind_t;

typedef enum {
    AS_BLK,
    AS_IF,
    AS_WHILE,
    AS_FOR,
    AS_RET,
    AS_ADD,
    AS_SUB,
    AS_MUL,
    AS_DIV,
    AS_MOD,
    AS_EQ,
    AS_NE,
    AS_LT,
    AS_LE,
    AS_GT,
    AS_GE,
    AS_ASG,
    AS_VAR,
    AS_NUM,
} askind_t;

struct tklist_t {
    tkkind_t kind;
    union {
        char *id;
        long long num;
    };
    tklist_t *next;
};

struct astree_t {
    askind_t kind;
    union {
        struct {
            astree_t *blk_body;
            astree_t *blk_next;
        };
        struct {
            astree_t *if_cond;
            astree_t *if_then;
            astree_t *if_else;
            size_t if_jmp;
        };
        struct {
            astree_t *while_cond;
            astree_t *while_body;
            size_t while_jmp;
        };
        struct {
            astree_t *for_init;
            astree_t *for_cond;
            astree_t *for_step;
            astree_t *for_body;
            size_t for_jmp;
        };
        struct {
            astree_t *ret_val;
        };
        struct {
            astree_t *bin_left;
            astree_t *bin_right;
        };
        struct {
            char *var_id;
            size_t var_ofs;
        };
        struct {
            long long num_val;
        };
    };
};

struct idlist_t {
    char *id;
    size_t ofs;
    idlist_t *next;
};

tklist_t *lexer(FILE *);
bool tklist_read(tklist_t **, tkkind_t);
bool tklist_match(tklist_t *, tkkind_t);
bool tklist_kind(tklist_t *, tkkind_t);
bool tklist_exist(tklist_t *);
void tklist_next(tklist_t **);
void tklist_show(tklist_t *);
void tklist_free(tklist_t *);

astree_t *parser(tklist_t *);
void astree_show(astree_t *);
void astree_free(astree_t *);

void generator(FILE *, astree_t *);

#endif
