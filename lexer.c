#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

tklist_t *lexer(FILE *);
bool tklist_read(tklist_t **, tkkind_t);
bool tklist_match(tklist_t *, tkkind_t);
bool tklist_kind(tklist_t *, tkkind_t);
bool tklist_exist(tklist_t *);
void tklist_next(tklist_t **);
void tklist_show(tklist_t *);
static void tklist_show_impl(tklist_t *);
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
    } else if (chr == '=') {
        chr = fgetc(ifp);
        if (chr == '=') {
            tkl->kind = TK_EQ;
        } else {
            ungetc(chr, ifp);
            tkl->kind = TK_ASG;
        }
    } else if (chr == '!') {
        chr = fgetc(ifp);
        if (chr == '=') {
            tkl->kind = TK_NE;
        } else {
            assert(false);
        }
    } else if (chr == '<') {
        chr = fgetc(ifp);
        if (chr == '=') {
            tkl->kind = TK_LE;
        } else {
            ungetc(chr, ifp);
            tkl->kind = TK_LT;
        }
    } else if (chr == '>') {
        chr = fgetc(ifp);
        if (chr == '=') {
            tkl->kind = TK_GE;
        } else {
            ungetc(chr, ifp);
            tkl->kind = TK_GT;
        }
    } else if (chr == '(') {
        tkl->kind = TK_LPRN;
    } else if (chr == ')') {
        tkl->kind = TK_RPRN;
    } else if (chr == '{') {
        tkl->kind = TK_LBRC;
    } else if (chr == '}') {
        tkl->kind = TK_RBRC;
    } else if (chr == ';') {
        tkl->kind = TK_SCLN;
    } else if (isdigit(chr)) {
        tkl->kind = TK_NUM;
        tkl->num = chr - '0';
        while (isdigit(chr = fgetc(ifp))) {
            tkl->num = tkl->num * 10 + chr - '0';
        }
        ungetc(chr, ifp);
    } else if (isalpha(chr) || chr == '_') {
        char *str = malloc(sizeof(char) * 16);
        assert(str != NULL);
        size_t len = 0, cap = 16;
        do {
            if (len == cap) {
                str = realloc(str, sizeof(char) * (cap *= 2));
                assert(str != NULL);
            }
            str[len++] = chr;
        } while (isalnum(chr = fgetc(ifp)) || chr == '_');
        str = realloc(str, sizeof(char) * (len + 1));
        assert(str != NULL);
        str[len] = '\0';
        ungetc(chr, ifp);
        if (strcmp(str, "if") == 0) {
            free(str);
            tkl->kind = TK_IF;
        } else if (strcmp(str, "else") == 0) {
            free(str);
            tkl->kind = TK_ELSE;
        } else if (strcmp(str, "while") == 0) {
            free(str);
            tkl->kind = TK_WHILE;
        } else if (strcmp(str, "for") == 0) {
            free(str);
            tkl->kind = TK_FOR;
        } else if (strcmp(str, "return") == 0) {
            free(str);
            tkl->kind = TK_RET;
        } else {
            tkl->kind = TK_ID;
            tkl->id = str;
        }
    } else {
        assert(false);
    }
    tkl->next = lexer(ifp);
    return tkl;
}

bool tklist_read(tklist_t **tkl, tkkind_t kind) {
    return tklist_match(*tkl, kind) && (tklist_next(tkl), true);
}

bool tklist_match(tklist_t *tkl, tkkind_t kind) {
    return tklist_exist(tkl) && tklist_kind(tkl, kind);
}

bool tklist_kind(tklist_t *tkl, tkkind_t kind) {
    return tkl->kind == kind;
}

bool tklist_exist(tklist_t *tkl) {
    return tkl != NULL;
}

void tklist_next(tklist_t **tkl) {
    *tkl = (*tkl)->next;
    return;
}

void tklist_show(tklist_t *tkl) {
    fputs("tklist:", stdout);
    tklist_show_impl(tkl);
    putchar('\n');
    return;
}

void tklist_show_impl(tklist_t *tkl) {
    if (tkl == NULL) {
        return;
    }
    putchar(' ');
    putchar('(');
    switch (tkl->kind) {
    case TK_ADD:
        fputs("TK_ADD: '+'", stdout);
        break;
    case TK_SUB:
        fputs("TK_SUB: '-'", stdout);
        break;
    case TK_MUL:
        fputs("TK_MUL: '*'", stdout);
        break;
    case TK_DIV:
        fputs("TK_DIV: '/'", stdout);
        break;
    case TK_MOD:
        fputs("TK_MOD: '%'", stdout);
        break;
    case TK_EQ:
        fputs("TK_EQ: '=='", stdout);
        break;
    case TK_NE:
        fputs("TK_NE: '!='", stdout);
        break;
    case TK_LT:
        fputs("TK_LT: '<'", stdout);
        break;
    case TK_LE:
        fputs("TK_LE: '<='", stdout);
        break;
    case TK_GT:
        fputs("TK_GT: '>'", stdout);
        break;
    case TK_GE:
        fputs("TK_GE: '>='", stdout);
        break;
    case TK_ASG:
        fputs("TK_ASG: '='", stdout);
        break;
    case TK_LPRN:
        fputs("TK_LPRN: '('", stdout);
        break;
    case TK_RPRN:
        fputs("TK_RPRN: ')'", stdout);
        break;
    case TK_LBRC:
        fputs("TK_LBRC: '{'", stdout);
        break;
    case TK_RBRC:
        fputs("TK_RBRC: '}'", stdout);
        break;
    case TK_SCLN:
        fputs("TK_SCLN: ';'", stdout);
        break;
    case TK_IF:
        fputs("TK_IF: 'if'", stdout);
        break;
    case TK_ELSE:
        fputs("TK_ELSE: 'else'", stdout);
        break;
    case TK_WHILE:
        fputs("TK_WHILE: 'while'", stdout);
        break;
    case TK_FOR:
        fputs("TK_FOR: 'for'", stdout);
        break;
    case TK_RET:
        fputs("TK_RET: 'return'", stdout);
        break;
    case TK_ID:
        printf("TK_ID: '%s'", tkl->id);
        break;
    case TK_NUM:
        printf("TK_NUM: '%lld'", tkl->num);
        break;
    default:
        assert(false);
    }
    putchar(')');
    tklist_show_impl(tkl->next);
    return;
}

void tklist_free(tklist_t *tkl) {
    if (tkl->next != NULL) {
        tklist_free(tkl->next);
    }
    free(tkl);
    return;
}
