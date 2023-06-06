#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* code;

typedef enum TokenKind TokenKind;
typedef struct Token Token;

enum TokenKind {
    TK_RES,
    TK_ID,
    TK_NUM,
    TK_EOF,
};

struct Token {
    TokenKind kind;
    Token* next;
    char* str;
    int len;
    long val;
};

Token* token;

Token* new_token(TokenKind, char*, int, Token*);

void tokenize(void);

bool consume(char*);
void expect(char*);
long expect_num(void);
bool is_eof(void);

typedef enum NodeKind NodeKind;
typedef struct Node Node;

enum NodeKind {
    ND_ASG,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_BLOCK,
    ND_IFEL,
    ND_WHILE,
    ND_FOR,
    ND_RET,
    ND_NUM,
    ND_ID,
    ND_FND,
    ND_FNC,
    ND_VAR,
};

struct Node {
    NodeKind kind;
    Node* head;
    Node* next;
    Node* op1;
    Node* op2;
    Node* op3;
    Node* op4;
    char* name;
    int len;
    long val;
    int ofs;
    int label;
};

int jump;

Node* node_res(NodeKind, Node*, Node*);
Node* node_num(long);
Node* node_id(void);
Node* node_func(NodeKind, Node*);
Node* node_var(Node*);

typedef struct Var Var;

struct Var {
    Var* next;
    char* name;
    int len;
    int ofs;
};

Var* local;

Var* new_var(Node*);

Node* node[256];

void prog(void);
Node* func(void);
Node* stmt(void);
Node* expr(void);
Node* asg(void);
Node* equal(void);
Node* rel(void);
Node* add(void);
Node* mul(void);
Node* unary(void);
Node* prim(void);

const char* reg_arg[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_code(void);
void gen_func(Node*);
void gen_stmt(Node*);
void gen_expr(Node*);
void gen_bin(Node*);
void gen_var(Node*);

int main(int argc, char** argv) {
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

Token* new_token(TokenKind kind, char* str, int len, Token* tk) {
    Token* new = calloc(1, sizeof(Token));
    new->kind = kind;
    new->str = str;
    new->len = len;
    tk->next = new;
    return new;
}

void tokenize(void) {
    Token head;
    head.next = NULL;
    Token* tk = &head;

    while (*code) {
        if (isspace(*code)) {
            code++;
            continue;
        }

        if (!strncmp(code, "==", 2) || !strncmp(code, "!=", 2) || !strncmp(code, "<=", 2) || !strncmp(code, ">=", 2)) {
            tk = new_token(TK_RES, code, 2, tk);
            code += 2;
            continue;
        }

        if (strchr("+-*/()<>;={},", *code)) {
            tk = new_token(TK_RES, code++, 1, tk);
            continue;
        }

        if (!strncmp(code, "if", 2) && !(isalnum(code[2]) || code[2] == '_')) {
            tk = new_token(TK_RES, code, 2, tk);
            code += 2;
            continue;
        }

        if (!strncmp(code, "else", 4) && !(isalnum(code[4]) || code[4] == '_')) {
            tk = new_token(TK_RES, code, 4, tk);
            code += 4;
            continue;
        }

        if (!strncmp(code, "while", 5) && !(isalnum(code[5]) || code[5] == '_')) {
            tk = new_token(TK_RES, code, 5, tk);
            code += 5;
            continue;
        }

        if (!strncmp(code, "for", 3) && !(isalnum(code[3]) || code[3] == '_')) {
            tk = new_token(TK_RES, code, 3, tk);
            code += 3;
            continue;
        }

        if (!strncmp(code, "return", 6) && !(isalnum(code[6]) || code[6] == '_')) {
            tk = new_token(TK_RES, code, 6, tk);
            code += 6;
            continue;
        }

        if (isalpha(*code) || *code == '_') {
            tk = new_token(TK_ID, code, 0, tk);
            char* q = code;
            while (isalnum(*code) || *code == '_') code++;
            tk->len = code - q;
            continue;
        }

        if (isdigit(*code)) {
            tk = new_token(TK_NUM, code, 0, tk);
            char* q = code;
            tk->val = strtol(code, &code, 10);
            tk->len = code - q;
            continue;
        }

        fprintf(stderr, "unexpected charactor \'%c\'\n", *code);
        exit(1);
    }

    new_token(TK_EOF, code, 0, tk);
    token = head.next;
    return;
}

bool consume(char* op) {
    if (token->kind != TK_RES || token->len != strlen(op) || strncmp(token->str, op, strlen(op))) {
        return false;
    }

    Token* del = token;
    token = token->next;
    free(del);
    return true;
}

void expect(char* op) {
    if (token->kind != TK_RES || token->len != strlen(op) || strncmp(token->str, op, strlen(op))) {
        fprintf(stderr, "\'%.*s\' is not \'%s\'\n", token->len, token->str, op);
        exit(1);
    }

    Token* del = token;
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
    Token* del = token;
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

Node* node_res(NodeKind kind, Node* op1, Node* op2) {
    Node* nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->op1 = op1;
    nd->op2 = op2;
    return nd;
}

Node* node_num(long val) {
    Node* nd = calloc(1, sizeof(Node));
    nd->kind = ND_NUM;
    nd->val = val;
    return nd;
}

Node* node_id(void) {
    Node* nd = calloc(1, sizeof(Node));
    nd->kind = ND_ID;
    nd->name = token->str;
    nd->len = token->len;
    Token* del = token;
    token = token->next;
    free(del);
    return nd;
}

Node* node_func(NodeKind kind, Node* nd) {
    Node* func = calloc(1, sizeof(Node));
    func->kind = kind;
    func->name = nd->name;
    func->len = nd->len;
    func->val = 0;
    Node* arg;

    while (!consume(")")) {
        arg = expr();
        arg->next = func->head;
        func->head = arg;
        func->val++;
        if (consume(",")) continue;
        if (consume(")")) break;
        fprintf(stderr, "expected ',' or ')'\n");
        exit(1);
    }

    return func;
}

Node* node_var(Node* nd) {
    Var* var = new_var(nd);
    nd->kind = ND_VAR;
    nd->ofs = var->ofs;
    return nd;
}

Var* new_var(Node* nd) {
    for (Var* var = local; var; var = var->next) {
        if (var->len == nd->len && !strncmp(var->name, nd->name, nd->len)) {
            return var;
        }
    }

    Var* var = calloc(1, sizeof(Var));
    var->name = nd->name;
    var->len = nd->len;
    var->ofs = local->ofs + 8;
    var->next = local;
    local = var;
    return var;
}

void prog(void) {
    int i = 0;

    while (!is_eof()) {
        node[i++] = func();
    }

    node[i] = NULL;
    return;
}

Node* func(void) {
    if (token->kind != TK_ID) {
        fprintf(stderr, "expected function name\n");
        exit(1);
    }

    local = calloc(1, sizeof(Var));
    Node* nd = node_id();
    expect("(");
    nd = node_func(ND_FND, nd);
    nd->op1 = stmt();
    nd->ofs = local->ofs;
    free(local);
    return nd;
}

Node* stmt(void) {
    if (consume("{")) {
        Node* nd = calloc(1, sizeof(Node));
        nd->kind = ND_BLOCK;
        Node* item = calloc(1, sizeof(Node));
        nd->head = item;

        while (!consume("}")) {
            item->next = stmt();
            item = item->next;
        }

        Node* del = nd->head;
        nd->head = nd->head->next;
        free(del);
        return nd;
    }

    if (consume("if")) {
        Node* nd = calloc(1, sizeof(Node));
        nd->kind = ND_IFEL;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(")");
        nd->op2 = stmt();

        if (consume("else")) {
            nd->op3 = stmt();
        } else {
            nd->op3 = node_num(0);
        }

        return nd;
    }

    if (consume("while")) {
        Node* nd = calloc(1, sizeof(Node));
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
        Node* nd = calloc(1, sizeof(Node));
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
        Node* nd = calloc(1, sizeof(Node));
        nd->kind = ND_RET;
        nd->op1 = expr();
        expect(";");
        return nd;
    }

    Node* nd = expr();
    expect(";");
    return nd;
}

Node* expr(void) {
    return asg();
}

Node* asg(void) {
    Node* nd = equal();

    if (consume("=")) {
        nd = node_res(ND_ASG, nd, asg());
    }

    return nd;
}

Node* equal(void) {
    Node* nd = rel();

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

Node* rel(void) {
    Node* nd = add();

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

Node* add(void) {
    Node* nd = mul();

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

Node* mul(void) {
    Node* nd = unary();

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

Node* unary(void) {
    if (consume("+")) {
        return node_res(ND_ADD, node_num(0), unary());
    }

    if (consume("-")) {
        return node_res(ND_SUB, node_num(0), unary());
    }

    return prim();
}

Node* prim(void) {
    if (consume("(")) {
        Node* nd = expr();
        expect(")");
        return nd;
    }

    if (token->kind == TK_ID) {
        Node* nd = node_id();

        if (consume("(")) {
            return node_func(ND_FNC, nd);
        }

        return node_var(nd);
    }

    return node_num(expect_num());
}

void gen_code(void) {
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    for (int i = 0; node[i]; i++) {
        gen_func(node[i]);
    }

    return;
}

void gen_func(Node* nd) {
    printf("\n%.*s:\n", nd->len, nd->name);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", nd->ofs);

    for (int i = 0; i < nd->val; i++) {
        if (i < 6) {
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", (i + 1) * 8);
            printf("    mov [rax], %s\n", reg_arg[i]);
        } else {
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", (i + 1) * 8);
            printf("    mov rdi, rbp\n");
            printf("    add rdi, %d\n", (i - 4) * 8);
            printf("    mov rdi, [rdi]\n");
            printf("    mov [rax], rdi\n");
        }
    }

    gen_stmt(nd->op1);
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
}

void gen_stmt(Node* nd) {
    switch (nd->kind) {
        case ND_BLOCK:
            for (Node* cur = nd->head; cur; cur = cur->next) {
                gen_stmt(cur);
            }

            break;

        case ND_IFEL:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label);
            gen_stmt(nd->op2);
            printf("    jmp .L%d\n", nd->label + 1);
            printf("\n.L%d:\n", nd->label);
            gen_stmt(nd->op3);
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

void gen_expr(Node* nd) {
    switch (nd->kind) {
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

            for (Node* cur = nd->head; cur; cur = cur->next) {
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
            break;

        case ND_VAR:
            gen_var(nd);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;

        case ND_ASG:
            gen_var(nd->op1);
            gen_expr(nd->op2);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            free(nd);
            break;

        default:
            gen_bin(nd);
            break;
    }

    return;
}

void gen_bin(Node* nd) {
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

void gen_var(Node* nd) {
    if (nd->kind != ND_VAR) {
        fprintf(stderr, "lvalue is not variable\n");
        exit(1);
    }

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", nd->ofs);
    printf("    push rax\n");
    free(nd);
    return;
}
