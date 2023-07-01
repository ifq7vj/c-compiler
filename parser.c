#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

astree_t *parser(tklist_t *);
static astree_t *parser_expr(tklist_t **);
static astree_t *parser_term(tklist_t **);
static astree_t *parser_unary(tklist_t **);
static astree_t *parser_factor(tklist_t **);
static astree_t *astree_newbin(askind_t, astree_t *, astree_t *);
static astree_t *astree_newnum(long long);
void astree_show(const char *, astree_t *);
static void astree_show_impl(const char *, astree_t *);
static void askind_show(astree_t *);
static void asstr_show(astree_t *);
void astree_free(astree_t *);

astree_t *parser(tklist_t *tkl) {
    return parser_expr(&tkl);
}

astree_t *parser_expr(tklist_t **tkl) {
    astree_t *ast = parser_term(tkl);
    while (true) {
        if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_ADD, ast, parser_term(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_SUB, ast, parser_term(tkl));
        } else {
            return ast;
        }
    }
}

astree_t *parser_term(tklist_t **tkl) {
    astree_t *ast = parser_unary(tkl);
    while (true) {
        if (*tkl != NULL && (*tkl)->kind == TK_MUL) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_MUL, ast, parser_unary(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_DIV) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_DIV, ast, parser_unary(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_MOD) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_MOD, ast, parser_unary(tkl));
        } else {
            return ast;
        }
    }
}

astree_t *parser_unary(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_ADD, astree_newnum(0), parser_unary(tkl));
    } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_SUB, astree_newnum(0), parser_unary(tkl));
    } else {
        return parser_factor(tkl);
    }
}

astree_t *parser_factor(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_NUM) {
        astree_t *ast = astree_newnum((*tkl)->num);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_LPAR) {
        *tkl = (*tkl)->next;
        astree_t *ast = parser_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RPAR);
        *tkl = (*tkl)->next;
        return ast;
    } else {
        assert(false);
    }
}

astree_t *astree_newbin(askind_t kind, astree_t *lhs, astree_t *rhs) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = kind;
    ast->lhs = lhs;
    ast->rhs = rhs;
    return ast;
}

astree_t *astree_newnum(long long num) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_NUM;
    ast->num = num;
    return ast;
}

void astree_show(const char *fmt, astree_t *ast) {
    fputs("astree:", stdout);
    astree_show_impl(fmt, ast);
    putchar('\n');
    return;
}

void astree_show_impl(const char *fmt, astree_t *ast) {
    if (ast == NULL) {
        return;
    }
    putchar(' ');
    for (const char *ptr = fmt; *ptr != '\0'; ++ptr) {
        if (*ptr == '%') {
            if (*++ptr == 'k') {
                askind_show(ast);
            } else if (*ptr == 's') {
                asstr_show(ast);
            } else if (*ptr == '%') {
                putchar('%');
            } else {
                assert(false);
            }
        } else {
            putchar(*ptr);
        }
    }
    astree_show_impl(fmt, ast->lhs);
    astree_show_impl(fmt, ast->rhs);
    return;
}

void askind_show(astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        fputs("AS_ADD", stdout);
        break;
    case AS_SUB:
        fputs("AS_SUB", stdout);
        break;
    case AS_MUL:
        fputs("AS_MUL", stdout);
        break;
    case AS_DIV:
        fputs("AS_DIV", stdout);
        break;
    case AS_MOD:
        fputs("AS_MOD", stdout);
        break;
    case AS_NUM:
        fputs("AS_NUM", stdout);
        break;
    default:
        assert(false);
    }
    return;
}

void asstr_show(astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        fputs("+", stdout);
        break;
    case AS_SUB:
        fputs("-", stdout);
        break;
    case AS_MUL:
        fputs("*", stdout);
        break;
    case AS_DIV:
        fputs("/", stdout);
        break;
    case AS_MOD:
        fputs("%", stdout);
        break;
    case AS_NUM:
        printf("%lld", ast->num);
        break;
    default:
        assert(false);
    }
    return;
}

void astree_free(astree_t *ast) {
    if (ast->lhs != NULL) {
        astree_free(ast->lhs);
    }
    if (ast->rhs != NULL) {
        astree_free(ast->rhs);
    }
    free(ast);
    return;
}
