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
    long val;
};

Token* token;

Token* new_token(TokenKind, char*, Token*);

Token* tokenize(char*);

bool consume(char);
long expect_number(void);
bool is_eof(void);

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: <compiler> <source>\n");
        exit(1);
    }

    token = tokenize(argv[1]);

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    printf("\nmain:\n");
    printf("    mov rax, %ld\n", expect_number());

    while (!is_eof())
    {
        if (consume('+'))
        {
            printf("    add rax, %ld\n", expect_number());
            continue;
        }

        if (consume('-'))
        {
            printf("    sub rax, %ld\n", expect_number());
            continue;
        }

        fprintf(stderr, "Error: Unexpected token \'%c\'\n", token->str[0]);
        exit(1);
    }

    printf("    ret\n");

    return 0;
}

Token* new_token(TokenKind kind, char* str, Token* cur)
{
    Token* tk = calloc(1, sizeof(Token));
    tk->kind = kind;
    tk->str = str;
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

        if (strchr("+-", *p))
        {
            cur = new_token(TK_RESERVED, p++, cur);
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, p, cur);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        fprintf(stderr, "Error: Unexpected charactor \'%c\'\n", *p);
        exit(1);
    }

    new_token(TK_EOF, p, cur);
    return head.next;
}

bool consume(char op)
{
    if (token->kind != TK_RESERVED || token->str[0] != op)
    {
        return false;
    }

    Token* del = token;
    token = token->next;
    free(del);
    return true;
}

long expect_number(void)
{
    if (token->kind != TK_NUM)
    {
        fprintf(stderr, "Error: \'%c\' is not number\n", token->str[0]);
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
