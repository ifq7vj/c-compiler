#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

extern char *code;
Token *token;

const char *tk_res[] = {"return", "while", "else", "for", "int", "if"};
const char *tk_op[] = {"!=", "<=", "==", ">=", "&", "(", ")", "*", "+", ",", "-", "/", ";", "<", "=", ">", "{", "}"};

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
            if (strncmp(code, tk_res[i], strlen(tk_res[i])) || isalnum(code[strlen(tk_res[i])]) || code[strlen(tk_res[i])] == '_') {
                continue;
            }

            tk = new_token(TK_RES, code, strlen(tk_res[i]), tk);
            code += strlen(tk_res[i]);
            flag = true;
        }

        if (flag) {
            continue;
        }

        for (int i = 0, n = sizeof(tk_op) / sizeof(char *); i < n; i++) {
            if (strncmp(code, tk_op[i], strlen(tk_op[i]))) {
                continue;
            }

            tk = new_token(TK_RES, code, strlen(tk_op[i]), tk);
            code += strlen(tk_op[i]);
            flag = true;
        }

        if (flag) {
            continue;
        }

        if (isalpha(*code) || *code == '_') {
            tk = new_token(TK_ID, code, 0, tk);
            char *ptr = code;

            while (isalnum(*code) || *code == '_') {
                code++;
            }

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

bool compare(char *op) {
    return token->kind == TK_RES && token->len == strlen(op) && !strncmp(token->str, op, strlen(op));
}

bool consume(char *op) {
    if (!compare(op)) {
        return false;
    }

    token = token->next;
    return true;
}

void expect(char *op) {
    if (!compare(op)) {
        fprintf(stderr, "\'%.*s\' is not \'%s\'\n", token->len, token->str, op);
        exit(1);
    }

    token = token->next;
    return;
}

long expect_num(void) {
    if (token->kind != TK_NUM) {
        fprintf(stderr, "\'%.*s\' is not number\n", token->len, token->str);
        exit(1);
    }

    long val = token->val;
    token = token->next;
    return val;
}

bool is_eof(void) {
    if (token->kind != TK_EOF) {
        return false;
    }

    return true;
}
