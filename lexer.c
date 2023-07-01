#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

tklist_t *lexer(FILE *);
void tklist_show(const char *, tklist_t *);
static void tklist_show_rec(const char *, tklist_t *);
static void tkkind_show(tklist_t *);
static void tkstr_show(tklist_t *);
void tklist_free(tklist_t *);

tklist_t *lexer(FILE *ifp) {
    int chr;
    do {
        chr = fgetc(ifp);
    } while (isspace(chr));
    if (chr == EOF) {
        return NULL;
    }
    tklist_t *tkl = malloc(sizeof(tklist_t));
    assert(tkl != NULL);
    if (chr == '+') {
        tkl->kind = TK_ADD;
    } else if (chr == '-') {
        tkl->kind = TK_SUB;
    } else if (chr == '*') {
        tkl->kind = TK_MUL;
    } else if (chr == '/') {
        tkl->kind = TK_DIV;
    } else if (chr == '%') {
        tkl->kind = TK_MOD;
    } else if (chr == '(') {
        tkl->kind = TK_LPAR;
    } else if (chr == ')') {
        tkl->kind = TK_RPAR;
    } else if (isdigit(chr)) {
        tkl->kind = TK_NUM;
        tkl->num = chr - '0';
        while (isdigit(chr = fgetc(ifp))) {
            tkl->num = tkl->num * 10 + chr - '0';
        }
        ungetc(chr, ifp);
    } else {
        assert(false);
    }
    tkl->next = lexer(ifp);
    return tkl;
}

void tklist_show(const char *fmt, tklist_t *tkl) {
    fputs("tklist:", stdout);
    tklist_show_rec(fmt, tkl);
    putchar('\n');
    return;
}

void tklist_show_rec(const char *fmt, tklist_t *tkl) {
    putchar(' ');
    for (const char *ptr = fmt; *ptr != '\0'; ++ptr) {
        if (*ptr == '%') {
            if (*++ptr == 'k') {
                tkkind_show(tkl);
            } else if (*ptr == 's') {
                tkstr_show(tkl);
            } else if (*ptr == '%') {
                putchar('%');
            } else {
                assert(false);
            }
        } else {
            putchar(*ptr);
        }
    }
    if (tkl->next != NULL) {
        tklist_show_rec(fmt, tkl->next);
    }
    return;
}

void tkkind_show(tklist_t *tkl) {
    switch (tkl->kind) {
    case TK_ADD:
        fputs("TK_ADD", stdout);
        return;
    case TK_SUB:
        fputs("TK_SUB", stdout);
        return;
    case TK_MUL:
        fputs("TK_MUL", stdout);
        return;
    case TK_DIV:
        fputs("TK_DIV", stdout);
        return;
    case TK_MOD:
        fputs("TK_MOD", stdout);
        return;
    case TK_LPAR:
        fputs("TK_LPAR", stdout);
        return;
    case TK_RPAR:
        fputs("TK_RPAR", stdout);
        return;
    case TK_NUM:
        fputs("TK_NUM", stdout);
        return;
    default:
        assert(false);
    }
}

void tkstr_show(tklist_t *tkl) {
    switch (tkl->kind) {
    case TK_ADD:
        fputs("+", stdout);
        return;
    case TK_SUB:
        fputs("-", stdout);
        return;
    case TK_MUL:
        fputs("*", stdout);
        return;
    case TK_DIV:
        fputs("/", stdout);
        return;
    case TK_MOD:
        fputs("%%", stdout);
        return;
    case TK_LPAR:
        fputs("(", stdout);
        return;
    case TK_RPAR:
        fputs(")", stdout);
        return;
    case TK_NUM:
        printf("%lld", tkl->num);
        return;
    default:
        assert(false);
    }
}

void tklist_free(tklist_t *tkl) {
    if (tkl->next != NULL) {
        tklist_free(tkl->next);
    }
    free(tkl);
    return;
}
