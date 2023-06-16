#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token Token;
typedef struct Node Node;
typedef struct Var Var;
typedef struct Type Type;

typedef enum {
    TK_RES, TK_ID, TK_NUM, TK_EOF,
} TokenKind;

typedef enum {
    ND_BLK, ND_IFEL, ND_WHILE, ND_FOR, ND_RET, ND_ID, ND_FND, ND_FNC, ND_VAR, ND_NUM,
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
    long val;
    int ofs;
    int label;
    Node *head, *next;
    Node *op1, *op2, *op3, *op4;
};

struct Var {
    Type *type;
    char *name;
    int len;
    int ofs;
    Var *next;
};

struct Type {
    TypeKind kind;
    Type *ptr;
};

void tokenize(void);

Token *new_token(TokenKind, char *, int, Token *);

bool consume(char *);
void expect(char *);
long expect_num(void);
bool is_eof(void);

void prog(void);
Node *func(void);
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
Node *node_id(void);
Node *node_func(NodeKind, Node *);
Node *node_vardef(Node *);
Node *node_varref(Node *);
Var *new_var(Node *);
Var *find_var(Node *);

void gen_code(void);
void gen_func(Node *);
void gen_stmt(Node *);
void gen_expr(Node *);
void gen_bin(Node *);

const char *tk_res[] = {"return", "while", "else", "for", "int", "if"};
const char *tk_op[] = {"!=", "<=", "==", ">=", "&", "(", ")", "*", "+", ",", "-", "/", ";", "<", "=", ">", "{", "}"};
const char *reg_arg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *code;
Token *token;
Node *node[256];
Var *local;
int jump;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    code = argv[1];
    jump = 0;
    tokenize();
    prog();
    gen_code();
    return 0;
}

void tokenize(void) {
    Token head;
    head.next = NULL;
    Token *tk = &head;

    while (*code) {
        if (isspace(*code)) {
            code++;
            continue;
        }

        bool flag = false;

        for (int i = 0, n = sizeof(tk_res) / sizeof(char *); i < n; i++) {
            if (!strncmp(code, tk_res[i], strlen(tk_res[i])) && !(isalnum(code[strlen(tk_res[i])]) || code[strlen(tk_res[i])] == '_')) {
                tk = new_token(TK_RES, code, strlen(tk_res[i]), tk);
                code += strlen(tk_res[i]);
                flag = true;
                break;
            }
        }

        if (flag) continue;

        for (int i = 0, n = sizeof(tk_op) / sizeof(char *); i < n; i++) {
            if (!strncmp(code, tk_op[i], strlen(tk_op[i]))) {
                tk = new_token(TK_RES, code, strlen(tk_op[i]), tk);
                code += strlen(tk_op[i]);
                flag = true;
                break;
            }
        }

        if (flag) continue;

        if (isalpha(*code) || *code == '_') {
            tk = new_token(TK_ID, code, 0, tk);
            char *ptr = code;
            while (isalnum(*code) || *code == '_') code++;
            tk->len = code - ptr;
            continue;
        }

        if (isdigit(*code)) {
            tk = new_token(TK_NUM, code, 0, tk);
            char *ptr = code;
            tk->val = strtol(code, &code, 10);
            tk->len = code - ptr;
            continue;
        }

        fprintf(stderr, "unexpected charactor \'%c\'\n", *code);
        exit(1);
    }

    new_token(TK_EOF, code, 0, tk);
    token = head.next;
    return;
}

Token *new_token(TokenKind kind, char *str, int len, Token *tk) {
    Token *new = calloc(1, sizeof(Token));
    new->kind = kind;
    new->str = str;
    new->len = len;
    tk->next = new;
    return new;
}

bool consume(char *op) {
    if (token->kind != TK_RES || token->len != strlen(op) || strncmp(token->str, op, strlen(op))) {
        return false;
    }

    Token *del = token;
    token = token->next;
    free(del);
    return true;
}

void expect(char *op) {
    if (token->kind != TK_RES || token->len != strlen(op) || strncmp(token->str, op, strlen(op))) {
        fprintf(stderr, "\'%.*s\' is not \'%s\'\n", token->len, token->str, op);
        exit(1);
    }

    Token *del = token;
    token = token->next;
    free(del);
    return;
}

long expect_num(void) {
    if (token->kind != TK_NUM) {
        fprintf(stderr, "\'%.*s\' is not number\n", token->len, token->str);
        exit(1);
    }

    long val = token->val;
    Token *del = token;
    token = token->next;
    free(del);
    return val;
}

bool is_eof(void) {
    if (token->kind != TK_EOF) {
        return false;
    }

    free(token);
    return true;
}

void prog(void) {
    int i = 0;

    while (!is_eof()) {
        node[i++] = func();
    }

    node[i] = NULL;
    return;
}

Node *func(void) {
    local = calloc(1, sizeof(Var));
    expect("int");
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;

    while (consume("*")) {
        Type *new = calloc(1, sizeof(Type));
        new->kind = TY_PTR;
        new->ptr = ty;
        ty = new;
    }

    Node *nd = node_id();
    expect("(");
    nd = node_func(ND_FND, nd);
    nd->op1 = stmt();
    nd->ofs = local->ofs;

    do {
        Var *del = local;
        local = local->next;
        free(del);
    } while (local);

    return nd;
}

Node *stmt(void) {
    if (consume("int")) {
        Type *ty = calloc(1, sizeof(Type));
        ty->kind = TY_INT;

        while (consume("*")) {
            Type *new = calloc(1, sizeof(Type));
            new->kind = TY_PTR;
            new->ptr = ty;
            ty = new;
        }

        Node *nd = node_id();
        expect(";");
        return node_vardef(nd);
    }

    if (consume("{")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_BLK;
        Node *item = calloc(1, sizeof(Node));
        nd->head = item;

        while (!consume("}")) {
            item->next = stmt();
            item = item->next;
        }

        Node *del = nd->head;
        nd->head = nd->head->next;
        free(del);
        return nd;
    }

    if (consume("if")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_IFEL;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(")");
        nd->op2 = stmt();

        if (consume("else")) {
            nd->op3 = stmt();
        }

        return nd;
    }

    if (consume("while")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_WHILE;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(")");
        nd->op2 = stmt();
        return nd;
    }

    if (consume("for")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_FOR;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(";");
        nd->op2 = expr();
        expect(";");
        nd->op3 = expr();
        expect(")");
        nd->op4 = stmt();
        return nd;
    }

    if (consume("return")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_RET;
        nd->op1 = expr();
        expect(";");
        return nd;
    }

    Node *nd = expr();
    expect(";");
    return nd;
}

Node *expr(void) {
    return asg();
}

Node *asg(void) {
    Node *nd = equal();

    if (consume("=")) {
        nd = node_res(ND_ASG, nd, asg());
    }

    return nd;
}

Node *equal(void) {
    Node *nd = rel();

    while (true) {
        if (consume("==")) {
            nd = node_res(ND_EQ, nd, rel());
            continue;
        }

        if (consume("!=")) {
            nd = node_res(ND_NE, nd, rel());
            continue;
        }

        return nd;
    }
}

Node *rel(void) {
    Node *nd = add();

    while (true) {
        if (consume("<")) {
            nd = node_res(ND_LT, nd, add());
            continue;
        }

        if (consume("<=")) {
            nd = node_res(ND_LE, nd, add());
            continue;
        }

        if (consume(">")) {
            nd = node_res(ND_LT, add(), nd);
            continue;
        }

        if (consume(">=")) {
            nd = node_res(ND_LE, add(), nd);
            continue;
        }

        return nd;
    }
}

Node *add(void) {
    Node *nd = mul();

    while (true) {
        if (consume("+")) {
            nd = node_res(ND_ADD, nd, mul());
            continue;
        }

        if (consume("-")) {
            nd = node_res(ND_SUB, nd, mul());
            continue;
        }

        return nd;
    }
}

Node *mul(void) {
    Node *nd = unary();

    while (true) {
        if (consume("*")) {
            nd = node_res(ND_MUL, nd, unary());
            continue;
        }

        if (consume("/")) {
            nd = node_res(ND_DIV, nd, unary());
            continue;
        }

        return nd;
    }
}

Node *unary(void) {
    if (consume("&")) {
        return node_res(ND_ADR, unary(), NULL);
    }

    if (consume("*")) {
        return node_res(ND_DER, unary(), NULL);
    }

    if (consume("+")) {
        return node_res(ND_ADD, node_num(0), unary());
    }

    if (consume("-")) {
        return node_res(ND_SUB, node_num(0), unary());
    }

    return prim();
}

Node *prim(void) {
    if (consume("(")) {
        Node *nd = expr();
        expect(")");
        return nd;
    }

    if (token->kind == TK_ID) {
        Node *nd = node_id();

        if (consume("(")) {
            return node_func(ND_FNC, nd);
        }

        return node_varref(nd);
    }

    return node_num(expect_num());
}

Node *node_res(NodeKind kind, Node *op1, Node *op2) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->op1 = op1;
    nd->op2 = op2;
    return nd;
}

Node *node_num(long val) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_NUM;
    nd->val = val;
    return nd;
}

Node *node_id(void) {
    if (token->kind != TK_ID) {
        fprintf(stderr, "\'%.*s\' is not identifier\n", token->len, token->str);
        exit(1);
    }

    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_ID;
    nd->name = token->str;
    nd->len = token->len;
    Token *del = token;
    token = token->next;
    free(del);
    return nd;
}

Node *node_func(NodeKind kind, Node *nd) {
    nd->kind = kind;
    nd->name = nd->name;
    nd->len = nd->len;
    nd->val = 0;
    Node *arg;

    while (!consume(")")) {
        if (kind == ND_FND) {
            expect("int");
            Type *ty = calloc(1, sizeof(Type));
            ty->kind = TY_INT;

            while (consume("*")) {
                Type *new = calloc(1, sizeof(Type));
                new->kind = TY_PTR;
                new->ptr = ty;
                ty = new;
            }

            arg = node_vardef(node_id());
        } else {
            arg = expr();
        }

        arg->next = nd->head;
        nd->head = arg;
        nd->val++;
        if (consume(",")) continue;
        if (consume(")")) break;
        fprintf(stderr, "expected ',' or ')'\n");
        exit(1);
    }

    return nd;
}

Node *node_vardef(Node *nd) {
    Var *var = new_var(nd);
    nd->kind = ND_VAR;
    nd->ofs = var->ofs;
    return nd;
}

Node *node_varref(Node *nd) {
    Var *var = find_var(nd);
    nd->kind = ND_VAR;
    nd->ofs = var->ofs;
    return nd;
}

Var *new_var(Node* nd) {
    for (Var *var = local; var; var = var->next) {
        if (var->len == nd->len && !strncmp(var->name, nd->name, nd->len)) {
            fprintf(stderr, "multiple definition of variable \'%.*s\'\n", nd->len, nd->name);
            exit(1);
        }
    }

    Var *var = calloc(1, sizeof(Var));
    var->name = nd->name;
    var->len = nd->len;
    var->ofs = local->ofs + 8;
    var->next = local;
    local = var;
    return var;
}

Var *find_var(Node *nd) {
    for (Var *var = local; var; var = var->next) {
        if (var->len == nd->len && !strncmp(var->name, nd->name, nd->len)) {
            return var;
        }
    }

    fprintf(stderr, "undefined variable \'%.*s\'\n", nd->len, nd->name);
    exit(1);
}

void gen_code(void) {
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    for (int i = 0; node[i]; i++) {
        gen_func(node[i]);
    }

    return;
}

void gen_func(Node *nd) {
    printf("\n%.*s:\n", nd->len, nd->name);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", nd->ofs);

    for (int i = 0; i < nd->val; i++) {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", (i + 1) * 8);

        if (i < 6) {
            printf("    mov [rax], %s\n", reg_arg[i]);
        } else {
            printf("    mov rdi, rbp\n");
            printf("    add rdi, %d\n", (i - 4) * 8);
            printf("    mov rdi, [rdi]\n");
            printf("    mov [rax], rdi\n");
        }
    }

    for (Node *cur = nd->head; cur; ) {
        Node *del = cur;
        cur = cur->next;
        gen_stmt(del);
    }

    gen_stmt(nd->op1);
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    free(nd);
    return;
}

void gen_stmt(Node *nd) {
    switch (nd->kind) {
        case ND_BLK:
            for (Node *cur = nd->head; cur; cur = cur->next) {
                gen_stmt(cur);
            }

            free(nd);
            break;

        case ND_IFEL:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label);
            gen_stmt(nd->op2);
            printf("    jmp .L%d\n", nd->label + 1);
            printf("\n.L%d:\n", nd->label);

            if (nd->op3) {
                gen_stmt(nd->op3);
            }

            printf("\n.L%d:\n", nd->label + 1);
            free(nd);
            break;

        case ND_WHILE:
            printf("\n.L%d:\n", nd->label);
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label + 1);
            gen_stmt(nd->op2);
            printf("    jmp .L%d\n", nd->label);
            printf("\n.L%d:\n", nd->label + 1);
            free(nd);
            break;

        case ND_FOR:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("\n.L%d:\n", nd->label);
            gen_expr(nd->op2);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label + 1);
            gen_stmt(nd->op4);
            gen_expr(nd->op3);
            printf("    pop rax\n");
            printf("    jmp .L%d\n", nd->label);
            printf("\n.L%d:\n", nd->label + 1);
            free(nd);
            break;

        case ND_RET:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            free(nd);
            break;

        default:
            gen_expr(nd);
            printf("    pop rax\n");
            break;
    }

    return;
}

void gen_expr(Node *nd) {
    switch (nd->kind) {
        case ND_ADR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->op1->ofs);
            printf("    push rax\n");
            free(nd->op1);
            free(nd);
            break;

        case ND_DER:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->op1->ofs);
            printf("    mov rax, [rax]\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            free(nd->op1);
            free(nd);
            break;

        case ND_ASG:
            switch (nd->op1->kind) {
                case ND_VAR:
                    printf("    mov rax, rbp\n");
                    printf("    sub rax, %d\n", nd->op1->ofs);
                    printf("    push rax\n");
                    break;

                case ND_DER:
                    printf("    mov rax, rbp\n");
                    printf("    sub rax, %d\n", nd->op1->op1->ofs);
                    printf("    mov rax, [rax]\n");
                    printf("    push rax\n");
                    free(nd->op1->op1);
                    break;

                default:
                    fprintf(stderr, "invalid lvalue\n");
                    exit(1);
            }

            gen_expr(nd->op2);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            free(nd);
            break;

        case ND_NUM:
            printf("    push %ld\n", nd->val);
            free(nd);
            break;

        case ND_FNC:
            printf("    push rsp\n");
            printf("    push [rsp]\n");

            if (nd->val > 6 && nd->val % 2 == 0) {
                printf("    add rsp, 8\n");
                printf("    and rsp, -16\n");
            } else {
                printf("    and rsp, -16\n");
                printf("    add rsp, 8\n");
            }

            for (Node *cur = nd->head; cur; cur = cur->next) {
                gen_expr(cur);
            }

            for (int i = 0; i < nd->val && i < 6; i++) {
                printf("    pop %s\n", reg_arg[i]);
            }

            printf("    call %.*s\n", nd->len, nd->name);

            for (int i = 6; i < nd->val; i++) {
                printf("    pop rdi\n");
            }

            printf("    mov rsp, [rsp]\n");
            printf("    push rax\n");
            free(nd);
            break;

        case ND_VAR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->ofs);
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;

        default:
            gen_bin(nd);
            break;
    }

    return;
}

void gen_bin(Node *nd) {
    gen_expr(nd->op1);
    gen_expr(nd->op2);
    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (nd->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;

        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;

        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;

        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;

        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;

        default:
            break;
    }

    printf("    push rax\n");
    free(nd);
    return;
}
