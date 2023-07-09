#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

astree_t *parser(tklist_t *);
static astree_t *parse_prog(tklist_t **);
static astree_t *parse_stmt(tklist_t **);
static astree_t *parse_expr(tklist_t **);
static astree_t *parse_add(tklist_t **);
static astree_t *parse_mul(tklist_t **);
static astree_t *parse_unary(tklist_t **);
static astree_t *parse_factor(tklist_t **);
static astree_t *astree_newbin(askind_t, astree_t *, astree_t *);
static astree_t *astree_newnum(long long);
static astree_t *astree_newvar(char *);
static idlist_t *idlist_newvar(char *, idlist_t *);
static idlist_t *idlist_findvar(char *, idlist_t *);
static void idlist_freevar(idlist_t *);
void astree_show(astree_t *);
static void astree_show_impl(astree_t *);
void astree_free(astree_t *);

idlist_t *local;

astree_t *parser(tklist_t *tkl) {
    local = malloc(sizeof(idlist_t));
    local->id = NULL;
    local->ofs = 0;
    local->next = NULL;
    astree_t *ast = parse_prog(&tkl);
    idlist_freevar(local);
    return ast;
}

astree_t *parse_prog(tklist_t **tkl) {
    if (*tkl == NULL) {
        return NULL;
    }
    astree_t *ast = parse_stmt(tkl);
    ast->next = parse_prog(tkl);
    return ast;
}

astree_t *parse_stmt(tklist_t **tkl) {
    astree_t *ast = parse_expr(tkl);
    assert(*tkl != NULL && (*tkl)->kind == TK_SCLN);
    *tkl = (*tkl)->next;
    return ast;
}

astree_t *parse_expr(tklist_t **tkl) {
    astree_t *ast = parse_add(tkl);
    if (*tkl != NULL && (*tkl)->kind == TK_ASG) {
        *tkl = (*tkl)->next;
        ast = astree_newbin(AS_ASG, ast, parse_expr(tkl));
    }
    return ast;
}

astree_t *parse_add(tklist_t **tkl) {
    astree_t *ast = parse_mul(tkl);
    while (true) {
        if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_ADD, ast, parse_mul(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_SUB, ast, parse_mul(tkl));
        } else {
            return ast;
        }
    }
}

astree_t *parse_mul(tklist_t **tkl) {
    astree_t *ast = parse_unary(tkl);
    while (true) {
        if (*tkl != NULL && (*tkl)->kind == TK_MUL) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_MUL, ast, parse_unary(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_DIV) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_DIV, ast, parse_unary(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_MOD) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_MOD, ast, parse_unary(tkl));
        } else {
            return ast;
        }
    }
}

astree_t *parse_unary(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_ADD, astree_newnum(0), parse_unary(tkl));
    } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_SUB, astree_newnum(0), parse_unary(tkl));
    } else {
        return parse_factor(tkl);
    }
}

astree_t *parse_factor(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_NUM) {
        astree_t *ast = astree_newnum((*tkl)->num);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_ID) {
        astree_t *ast = astree_newvar((*tkl)->id);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_LPAR) {
        *tkl = (*tkl)->next;
        astree_t *ast = parse_expr(tkl);
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
    ast->lhs = NULL;
    ast->rhs = NULL;
    return ast;
}

astree_t *astree_newvar(char *id) {
    idlist_t *idl = idlist_findvar(id, local);
    if (idl == NULL) {
        idl = local = idlist_newvar(id, local);
    }
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_VAR;
    ast->id = idl->id;
    ast->ofs = idl->ofs;
    ast->lhs = NULL;
    ast->rhs = NULL;
    return ast;
}

idlist_t *idlist_findvar(char *id, idlist_t *idl) {
    if (idl == NULL) {
        return NULL;
    }
    if (idl->id != NULL && strcmp(id, idl->id) == 0) {
        return idl;
    }
    return idlist_findvar(id, idl->next);
}

idlist_t *idlist_newvar(char *id, idlist_t *next) {
    idlist_t *idl = malloc(sizeof(idlist_t));
    idl->id = id;
    idl->ofs = next->ofs + 1;
    idl->next = next;
    return idl;
}

void idlist_freevar(idlist_t *idl) {
    if (idl == NULL) {
        return;
    }
    idlist_freevar(idl->next);
    free(idl);
    return;
}

void astree_show(astree_t *ast) {
    fputs("astree:", stdout);
    astree_show_impl(ast);
    putchar('\n');
    return;
}

void astree_show_impl(astree_t *ast) {
    if (ast == NULL) {
        return;
    }
    putchar(' ');
    putchar('(');
    switch (ast->kind) {
    case AS_ADD:
        fputs("AS_ADD:", stdout);
        break;
    case AS_SUB:
        fputs("AS_SUB:", stdout);
        break;
    case AS_MUL:
        fputs("AS_MUL:", stdout);
        break;
    case AS_DIV:
        fputs("AS_DIV:", stdout);
        break;
    case AS_MOD:
        fputs("AS_MOD:", stdout);
        break;
    case AS_ASG:
        fputs("AS_ASG:", stdout);
        break;
    case AS_NUM:
        printf("AS_NUM: '%lld'", ast->num);
        break;
    case AS_VAR:
        printf("AS_VAR: '%s'", ast->id);
        break;
    default:
        assert(false);
    }
    astree_show_impl(ast->lhs);
    astree_show_impl(ast->rhs);
    putchar(')');
    astree_show_impl(ast->next);
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
