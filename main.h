#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

typedef struct Token Token;
typedef struct Node Node;
typedef struct Func Func;
typedef struct Var Var;
typedef struct Type Type;

typedef enum {
    TK_RES, TK_ID, TK_NUM, TK_EOF,
} TokenKind;

typedef enum {
    ND_NOP, ND_BLK, ND_IFEL, ND_WHILE, ND_FOR, ND_RET, ND_FND, ND_FNC, ND_VAR, ND_NUM,
    ND_ASG, ND_EQ, ND_NE, ND_LT, ND_LE, ND_ADD, ND_SUB, ND_MUL, ND_DIV, ND_ADR, ND_DER,
} NodeKind;

typedef enum {
    TY_INT, TY_PTR,
} TypeKind;

struct Token {
    TokenKind kind;
    char *str;
    int len;
    long val;
    Token *next;
};

struct Node {
    NodeKind kind;
    char *name;
    int len;
    Type *type;
    int size;
    long val;
    int ofs;
    int label;
    Node *head, *next;
    Node *op1, *op2, *op3, *op4;
};

struct Func {
    char *name;
    int len;
    Type *type;
    Func *next;
};

struct Var {
    char *name;
    int len;
    Type *type;
    int ofs;
    Var *next;
};

struct Type {
    TypeKind kind;
    Type *ptr;
};

void tokenize(void);

Token *new_token(TokenKind, char *, int, Token *);

bool compare(char *);
bool consume(char *);
void expect(char *);
long expect_num(void);
bool is_eof(void);

void prog(void);
Node *glob(void);
Node *stmt(void);
Node *expr(void);
Node *asg(void);
Node *equal(void);
Node *rel(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *prim(void);

Node *node_res(NodeKind, Node *, Node *);
Node *node_num(long);
Node *node_func(void);
Node *node_var(void);
Func *new_func(char *name, int len, Type *);
Func *find_func(char *name, int len);
Var *new_var(char *name, int len, Type *);
Var *find_var(char *name, int len);
Type *new_type(void);
Type *copy_type(Type *);

void gen_code(void);
void gen_func(Node *);
void gen_stmt(Node *);
void gen_expr(Node *);
void gen_bin(Node *);
void gen_lval(Node *);

#endif
