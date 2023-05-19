#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum TokenKind TokenKind;
typedef struct Token Token;

enum TokenKind
{
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
};

struct Token
{
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
long expect_number(void);
bool is_eof(void);

typedef enum NodeKind NodeKind;
typedef struct Node Node;

enum NodeKind
{
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
};

struct Node
{
    NodeKind kind;
    Node* lhs;
    Node* rhs;
    long val;
};

Node* new_node(NodeKind, Node*, Node*);
Node* new_node_num(long val);

Node* expr(void);
Node* equal(void);
Node* rel(void);
Node* add(void);
Node* mul(void);
Node* unary(void);
Node* prim(void);

void gen(Node* node);

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    token = tokenize(argv[1]);
    Node* node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    printf("\nmain:\n");

    gen(node);

    printf("    pop rax\n");
    printf("    ret\n");

    return 0;
}

Token* new_token(TokenKind kind, char* str, int len, Token* cur)
{
    Token* tk = calloc(1, sizeof(Token));
    tk->kind = kind;
    tk->str = str;
    tk->len = len;
    cur->next = tk;
    return tk;
}

Token* tokenize(char* p)
{
    Token head;
    head.next = NULL;
    Token* cur = &head;

    while (*p)
    {
        if (isspace(*p))
        {
            p++;
            continue;
        }

        if (!strncmp(p, "==", 2) || !strncmp(p, "!=", 2) || !strncmp(p, "<=", 2) || !strncmp(p, ">=", 2))
        {
            cur = new_token(TK_RESERVED, p, 2, cur);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>", *p))
        {
            cur = new_token(TK_RESERVED, p++, 1, cur);
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, p, 0, cur);
            char* q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        fprintf(stderr, "Error: Unexpected charactor \'%c\'\n", *p);
        exit(1);
    }

    new_token(TK_EOF, p, 0, cur);
    return head.next;
}

bool consume(char* op)
{
    if (token->kind != TK_RESERVED || token->len != strlen(op) || strncmp(token->str, op, token->len))
    {
        return false;
    }

    Token* del = token;
    token = token->next;
    free(del);
    return true;
}

void expect(char* op)
{
    if (token->kind != TK_RESERVED || token->len != strlen(op) || strncmp(token->str, op, token->len))
    {
        fprintf(stderr, "Error: \'%.*s\' is not \'%s\'\n", (int)token->len, token->str, op);
        exit(1);
    }

    Token* del = token;
    token = token->next;
    free(del);
    return;
}

long expect_number(void)
{
    if (token->kind != TK_NUM)
    {
        fprintf(stderr, "Error: \'%.*s\' is not number\n", (int)token->len, token->str);
        exit(1);
    }

    Token* del = token;
    long val = token->val;
    token = token->next;
    free(del);
    return val;
}

bool is_eof(void)
{
    if (token->kind != TK_EOF)
    {
        return false;
    }

    free(token);
    return true;
}

Node* new_node(NodeKind kind, Node* lhs, Node* rhs)
{
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(long val)
{
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node* expr(void)
{
    return equal();
}

Node* equal(void)
{
    Node* node = rel();

    while (true)
    {
        if (consume("=="))
        {
            node = new_node(ND_EQ, node, rel());
            continue;
        }

        if (consume("!="))
        {
            node = new_node(ND_NE, node, rel());
            continue;
        }

        return node;
    }
}

Node* rel(void)
{
    Node* node = add();

    while (true)
    {
        if (consume("<"))
        {
            node = new_node(ND_LT, node, add());
            continue;
        }

        if (consume("<="))
        {
            node = new_node(ND_LE, node, add());
            continue;
        }

        if (consume(">"))
        {
            node = new_node(ND_LT, add(), node);
            continue;
        }

        if (consume(">="))
        {
            node = new_node(ND_LE, add(), node);
            continue;
        }

        return node;
    }
}

Node* add(void)
{
    Node* node = mul();

    while (true)
    {
        if (consume("+"))
        {
            node = new_node(ND_ADD, node, mul());
            continue;
        }

        if (consume("-"))
        {
            node = new_node(ND_SUB, node, mul());
            continue;
        }

        return node;
    }
}

Node* mul(void)
{
    Node* node = unary();

    while (true)
    {
        if (consume("*"))
        {
            node = new_node(ND_MUL, node, unary());
            continue;
        }

        if (consume("/"))
        {
            node = new_node(ND_DIV, node, unary());
            continue;
        }

        return node;
    }
}

Node* unary(void)
{
    if (consume("+"))
    {
        return new_node(ND_ADD, new_node_num(0), unary());
    }

    if (consume("-"))
    {
        return new_node(ND_SUB, new_node_num(0), unary());
    }

    return prim();
}

Node* prim(void)
{
    if (consume("("))
    {
        Node* node = expr();
        expect(")");
        return node;
    }

    return new_node_num(expect_number());
}

void gen(Node* node)
{
    if (node->kind == ND_NUM)
    {
        printf("    push %ld\n", node->val);
        free(node);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind)
    {
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
