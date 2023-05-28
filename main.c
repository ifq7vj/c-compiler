#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

Token* tokenize(char*);

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
    ND_VAR,
    ND_FNC,
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
Node* node_var(Node*);
Node* node_fnc(Node*);

typedef struct Var Var;

struct Var {
    Var* next;
    char* name;
    int len;
    int ofs;
};

Var* local;

Var* new_var(Node*);

Node* code[256];

void prog(void);
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
void gen_stmt(Node*);
void gen_expr(Node*);
void gen_var(Node*);

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    token = tokenize(argv[1]);
    local = calloc(1, sizeof(Var));
    jump = 0;
    prog();
    gen_code();
    return 0;
}

Token* new_token(TokenKind kind, char* str, int len, Token* cur) {
    Token* tk = calloc(1, sizeof(Token));
    tk->kind = kind;
    tk->str = str;
    tk->len = len;
    cur->next = tk;
    return tk;
}

Token* tokenize(char* p) {
    Token head;
    head.next = NULL;
    Token* cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (!strncmp(p, "==", 2) || !strncmp(p, "!=", 2) || !strncmp(p, "<=", 2) || !strncmp(p, ">=", 2)) {
            cur = new_token(TK_RES, p, 2, cur);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>;={},", *p)) {
            cur = new_token(TK_RES, p++, 1, cur);
            continue;
        }

        if (!strncmp(p, "if", 2) && !(isalnum(p[2]) || p[2] == '_')) {
            cur = new_token(TK_RES, p, 2, cur);
            p += 2;
            continue;
        }

        if (!strncmp(p, "else", 4) && !(isalnum(p[4]) || p[4] == '_')) {
            cur = new_token(TK_RES, p, 4, cur);
            p += 4;
            continue;
        }

        if (!strncmp(p, "while", 5) && !(isalnum(p[5]) || p[5] == '_')) {
            cur = new_token(TK_RES, p, 5, cur);
            p += 5;
            continue;
        }

        if (!strncmp(p, "for", 3) && !(isalnum(p[3]) || p[3] == '_')) {
            cur = new_token(TK_RES, p, 3, cur);
            p += 3;
            continue;
        }

        if (!strncmp(p, "return", 6) && !(isalnum(p[6]) || p[6] == '_')) {
            cur = new_token(TK_RES, p, 6, cur);
            p += 6;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            cur = new_token(TK_ID, p, 0, cur);
            char* q = p;
            while (isalnum(*p) || *p == '_') p++;
            cur->len = p - q;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, p, 0, cur);
            char* q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        fprintf(stderr, "unexpected charactor \'%c\'\n", *p);
        exit(1);
    }

    new_token(TK_EOF, p, 0, cur);
    return head.next;
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
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->op1 = op1;
    node->op2 = op2;
    return node;
}

Node* node_num(long val) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node* node_id(void) {
    char* name = token->str;
    int len = token->len;
    Token* del = token;
    token = token->next;
    free(del);
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_ID;
    node->name = name;
    node->len = len;
    return node;
}

Node* node_var(Node* node) {
    Var* var = new_var(node);
    node->kind = ND_VAR;
    node->ofs = var->ofs;
    return node;
}

Node* node_fnc(Node* node) {
    Node* func = calloc(1, sizeof(Node));
    func->kind = ND_FNC;
    func->name = node->name;
    func->len = node->len;
    func->val = 0;
    Node* arg;

    while (!consume(")")) {
        arg = asg();
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

Var* new_var(Node* node) {
    for (Var* var = local; var; var = var->next) {
        if (var->len == node->len && !strncmp(var->name, node->name, node->len)) {
            return var;
        }
    }

    Var* var = calloc(1, sizeof(Var));
    var->name = node->name;
    var->len = node->len;
    var->ofs = local->ofs + 8;
    var->next = local;
    local = var;
    return var;
}

void prog(void) {
    int i = 0;

    while (!is_eof()) {
        code[i++] = stmt();
    }

    code[i] = NULL;
    return;
}

Node* stmt(void) {
    if (consume("{")) {
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        Node* item = calloc(1, sizeof(Node));
        node->head = item;

        while (!consume("}")) {
            item->next = stmt();
            item = item->next;
        }

        Node* del = node->head;
        node->head = node->head->next;
        free(del);
        return node;
    }

    if (consume("if")) {
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_IFEL;
        node->label = jump;
        jump += 2;
        expect("(");
        node->op1 = expr();
        expect(")");
        node->op2 = stmt();

        if (consume("else")) {
            node->op3 = stmt();
        } else {
            node->op3 = node_num(0);
        }

        return node;
    }

    if (consume("while")) {
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        node->label = jump;
        jump += 2;
        expect("(");
        node->op1 = expr();
        expect(")");
        node->op2 = stmt();
        return node;
    }

    if (consume("for")) {
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        node->label = jump;
        jump += 2;
        expect("(");
        node->op1 = expr();
        expect(";");
        node->op2 = expr();
        expect(";");
        node->op3 = expr();
        expect(")");
        node->op4 = stmt();
        return node;
    }

    if (consume("return")) {
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_RET;
        node->op1 = expr();
        expect(";");
        return node;
    }

    Node* node = expr();
    expect(";");
    return node;
}

Node* expr(void) {
    return asg();
}

Node* asg(void) {
    Node* node = equal();

    if (consume("=")) {
        node = node_res(ND_ASG, node, asg());
    }

    return node;
}

Node* equal(void) {
    Node* node = rel();

    while (true) {
        if (consume("==")) {
            node = node_res(ND_EQ, node, rel());
            continue;
        }

        if (consume("!=")) {
            node = node_res(ND_NE, node, rel());
            continue;
        }

        return node;
    }
}

Node* rel(void) {
    Node* node = add();

    while (true) {
        if (consume("<")) {
            node = node_res(ND_LT, node, add());
            continue;
        }

        if (consume("<=")) {
            node = node_res(ND_LE, node, add());
            continue;
        }

        if (consume(">")) {
            node = node_res(ND_LT, add(), node);
            continue;
        }

        if (consume(">=")) {
            node = node_res(ND_LE, add(), node);
            continue;
        }

        return node;
    }
}

Node* add(void) {
    Node* node = mul();

    while (true) {
        if (consume("+")) {
            node = node_res(ND_ADD, node, mul());
            continue;
        }

        if (consume("-")) {
            node = node_res(ND_SUB, node, mul());
            continue;
        }

        return node;
    }
}

Node* mul(void) {
    Node* node = unary();

    while (true) {
        if (consume("*")) {
            node = node_res(ND_MUL, node, unary());
            continue;
        }

        if (consume("/")) {
            node = node_res(ND_DIV, node, unary());
            continue;
        }

        return node;
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
        Node* node = expr();
        expect(")");
        return node;
    }

    if (token->kind == TK_ID) {
        Node* node = node_id();

        if (consume("(")) {
            return node_fnc(node);
        }

        return node_var(node);
    }

    return node_num(expect_num());
}

void gen_code(void) {
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("\nmain:\n");
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", local->ofs);

    for (int i = 0; code[i]; i++) {
        gen_stmt(code[i]);
        printf("    pop rax\n");
    }

    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
}

void gen_stmt(Node* node) {
    switch (node->kind) {
        case ND_BLOCK:

            for (Node* cur = node->head; cur; cur = cur->next) {
                gen_stmt(cur);
                printf("    pop rax\n");
            }

            printf("    push 0\n");
            break;

        case ND_IFEL:
            gen_stmt(node->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", node->label);
            gen_stmt(node->op2);
            printf("    pop rax\n");
            printf("    jmp .L%d\n", node->label + 1);
            printf("\n.L%d:\n", node->label);
            gen_stmt(node->op3);
            printf("    pop rax\n");
            printf("\n.L%d:\n", node->label + 1);
            printf("    push 0\n");
            free(node);
            break;
        
        case ND_WHILE:
            printf("\n.L%d:\n", node->label);
            gen_stmt(node->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", node->label + 1);
            gen_stmt(node->op2);
            printf("    pop rax\n");
            printf("    jmp .L%d\n", node->label);
            printf("\n.L%d:\n", node->label + 1);
            printf("    push 0\n");
            free(node);
            break;

        case ND_FOR:
            gen_stmt(node->op1);
            printf("\n.L%d:\n", node->label);
            gen_stmt(node->op2);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", node->label + 1);
            gen_stmt(node->op4);
            printf("    pop rax\n");
            gen_stmt(node->op3);
            printf("    pop rax\n");
            printf("    jmp .L%d\n", node->label);
            printf("\n.L%d:\n", node->label + 1);
            printf("    push 0\n");
            free(node);
            break;

        case ND_RET:
            gen_stmt(node->op1);
            printf("    pop rax\n");
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            free(node);
            break;

        case ND_ASG:
            gen_var(node->op1);
            gen_stmt(node->op2);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            free(node);
            break;

        case ND_VAR:
            gen_var(node);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;
        
        case ND_NUM:
            printf("    push %ld\n", node->val);
            free(node);
            break;

        case ND_FNC:
            printf("    push rsp\n");
            printf("    push [rsp]\n");

            if (node->val > 6 && node->val % 2 == 0) {
                printf("    add rsp, 8\n");
                printf("    and rsp, -16\n");
            } else {
                printf("    and rsp, -16\n");
                printf("    add rsp, 8\n");
            }

            for (Node* cur = node->head; cur; cur = cur->next) {
                gen_stmt(cur);
            }
            
            for (int i = 0; i < node->val && i < 6; i++) {
                printf("    pop %s\n", reg_arg[i]);
            }
            
            printf("    call %.*s\n", node->len, node->name);
            
            for (int i = 6; i < node->val; i++) {
                printf("    pop rdi\n");
            }
            
            printf("    mov rsp, [rsp]\n");
            printf("    push rax\n");
            break;

        default:
            gen_expr(node);
            break;
    }

    return;
}

void gen_expr(Node* node) {
    gen_stmt(node->op1);
    gen_stmt(node->op2);
    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
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
    free(node);
    return;
}

void gen_var(Node* node) {
    if (node->kind != ND_VAR) {
        fprintf(stderr, "Error: Lvalue is not variable\n");
        exit(1);
    }

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->ofs);
    printf("    push rax\n");
    free(node);
    return;
}
