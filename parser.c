#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

astree_t *parser(tklist_t *);
static astree_t *parse_prog(tklist_t **);
static astree_t *parse_block(tklist_t **);
static astree_t *parse_stmt(tklist_t **);
static astree_t *parse_expr(tklist_t **);
static astree_t *parse_asg(tklist_t **);
static astree_t *parse_eq(tklist_t **);
static astree_t *parse_rel(tklist_t **);
static astree_t *parse_add(tklist_t **);
static astree_t *parse_mul(tklist_t **);
static astree_t *parse_unary(tklist_t **);
static astree_t *parse_prim(tklist_t **);
static astree_t *astree_newbin(askind_t, astree_t *, astree_t *);
static astree_t *astree_newif(astree_t *, astree_t *, astree_t *);
static astree_t *astree_newwhile(astree_t *, astree_t *);
static astree_t *astree_newfor(astree_t *, astree_t *, astree_t *, astree_t *);
static astree_t *astree_newret(astree_t *);
static astree_t *astree_newvar(char *);
static astree_t *astree_newnum(long long);
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
    astree_t *ast = parse_block(tkl);
    assert(*tkl == NULL);
    return ast;
}

astree_t *parse_block(tklist_t **tkl) {
    if (*tkl == NULL || (*tkl)->kind == TK_RBRC) {
        return NULL;
    }
    astree_t *ast = parse_stmt(tkl);
    ast->next = parse_block(tkl);
    return ast;
}

astree_t *parse_stmt(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_IF) {
        *tkl = (*tkl)->next;
        assert(*tkl != NULL && (*tkl)->kind == TK_LPRN);
        *tkl = (*tkl)->next;
        astree_t *if_cond = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RPRN);
        *tkl = (*tkl)->next;
        astree_t *if_then = parse_stmt(tkl);
        if (*tkl != NULL && (*tkl)->kind == TK_ELSE) {
            *tkl = (*tkl)->next;
            astree_t *if_else = parse_stmt(tkl);
            return astree_newif(if_cond, if_then, if_else);
        } else {
            return astree_newif(if_cond, if_then, NULL);
        }
    } else if (*tkl != NULL && (*tkl)->kind == TK_WHILE) {
        *tkl = (*tkl)->next;
        assert(*tkl != NULL && (*tkl)->kind == TK_LPRN);
        *tkl = (*tkl)->next;
        astree_t *while_cond = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RPRN);
        *tkl = (*tkl)->next;
        astree_t *while_body = parse_stmt(tkl);
        return astree_newwhile(while_cond, while_body);
    } else if (*tkl != NULL && (*tkl)->kind == TK_FOR) {
        *tkl = (*tkl)->next;
        assert(*tkl != NULL && (*tkl)->kind == TK_LPRN);
        *tkl = (*tkl)->next;
        astree_t *for_init = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_SCLN);
        *tkl = (*tkl)->next;
        astree_t *for_cond = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_SCLN);
        *tkl = (*tkl)->next;
        astree_t *for_step = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RPRN);
        *tkl = (*tkl)->next;
        astree_t *for_body = parse_stmt(tkl);
        return astree_newfor(for_init, for_cond, for_step, for_body);
    } else if (*tkl != NULL && (*tkl)->kind == TK_RET) {
        *tkl = (*tkl)->next;
        astree_t *ast = astree_newret(parse_expr(tkl));
        assert(*tkl != NULL && (*tkl)->kind == TK_SCLN);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_LBRC) {
        *tkl = (*tkl)->next;
        astree_t *ast = parse_block(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RBRC);
        *tkl = (*tkl)->next;
        return ast;
    } else {
        astree_t *ast = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_SCLN);
        *tkl = (*tkl)->next;
        return ast;
    }
}

astree_t *parse_expr(tklist_t **tkl) {
    return parse_asg(tkl);
}

astree_t *parse_asg(tklist_t **tkl) {
    astree_t *ast = parse_eq(tkl);
    if (*tkl != NULL && (*tkl)->kind == TK_ASG) {
        *tkl = (*tkl)->next;
        ast = astree_newbin(AS_ASG, ast, parse_expr(tkl));
    }
    return ast;
}

astree_t *parse_eq(tklist_t **tkl) {
    astree_t *ast = parse_rel(tkl);
    do {
        if (*tkl != NULL && (*tkl)->kind == TK_EQ) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_EQ, ast, parse_rel(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_NE) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_NE, ast, parse_rel(tkl));
        } else {
            return ast;
        }
    } while (true);
}

astree_t *parse_rel(tklist_t **tkl) {
    astree_t *ast = parse_add(tkl);
    do {
        if (*tkl != NULL && (*tkl)->kind == TK_LT) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_LT, ast, parse_add(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_LE) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_LE, ast, parse_add(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_GT) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_GT, ast, parse_add(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_GE) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_GE, ast, parse_add(tkl));
        } else {
            return ast;
        }
    } while (true);
}

astree_t *parse_add(tklist_t **tkl) {
    astree_t *ast = parse_mul(tkl);
    do {
        if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_ADD, ast, parse_mul(tkl));
        } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
            *tkl = (*tkl)->next;
            ast = astree_newbin(AS_SUB, ast, parse_mul(tkl));
        } else {
            return ast;
        }
    } while (true);
}

astree_t *parse_mul(tklist_t **tkl) {
    astree_t *ast = parse_unary(tkl);
    do {
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
    } while (true);
}

astree_t *parse_unary(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_ADD) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_ADD, astree_newnum(0), parse_unary(tkl));
    } else if (*tkl != NULL && (*tkl)->kind == TK_SUB) {
        *tkl = (*tkl)->next;
        return astree_newbin(AS_SUB, astree_newnum(0), parse_unary(tkl));
    } else {
        return parse_prim(tkl);
    }
}

astree_t *parse_prim(tklist_t **tkl) {
    if (*tkl != NULL && (*tkl)->kind == TK_NUM) {
        astree_t *ast = astree_newnum((*tkl)->num);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_ID) {
        astree_t *ast = astree_newvar((*tkl)->id);
        *tkl = (*tkl)->next;
        return ast;
    } else if (*tkl != NULL && (*tkl)->kind == TK_LPRN) {
        *tkl = (*tkl)->next;
        astree_t *ast = parse_expr(tkl);
        assert(*tkl != NULL && (*tkl)->kind == TK_RPRN);
        *tkl = (*tkl)->next;
        return ast;
    } else {
        assert(false);
    }
}

astree_t *astree_newbin(askind_t kind, astree_t *lhs, astree_t *rhs) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = kind;
    ast->bin_left = lhs;
    ast->bin_right = rhs;
    return ast;
}

astree_t *astree_newif(astree_t *if_cond, astree_t *if_then, astree_t *if_else) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_IF;
    ast->if_cond = if_cond;
    ast->if_then = if_then;
    ast->if_else = if_else;
    return ast;
}

astree_t *astree_newwhile(astree_t *while_cond, astree_t *while_body) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_WHILE;
    ast->while_cond = while_cond;
    ast->while_body = while_body;
    return ast;
}

astree_t *astree_newfor(astree_t *for_init, astree_t *for_cond, astree_t *for_step, astree_t *for_body) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_FOR;
    ast->for_init = for_init;
    ast->for_cond = for_cond;
    ast->for_step = for_step;
    ast->for_body = for_body;
    return ast;
}

astree_t *astree_newret(astree_t *val) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_RET;
    ast->ret_val = val;
    return ast;
}

astree_t *astree_newvar(char *id) {
    idlist_t *idl = idlist_findvar(id, local);
    if (idl == NULL) {
        idl = local = idlist_newvar(id, local);
    }
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_VAR;
    ast->var_id = idl->id;
    ast->var_ofs = idl->ofs;
    return ast;
}

astree_t *astree_newnum(long long num) {
    astree_t *ast = malloc(sizeof(astree_t));
    ast->kind = AS_NUM;
    ast->num_val = num;
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
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_SUB:
        fputs("AS_SUB:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_MUL:
        fputs("AS_MUL:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_DIV:
        fputs("AS_DIV:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_MOD:
        fputs("AS_MOD:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_EQ:
        fputs("AS_EQ:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_NE:
        fputs("AS_NE:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_LT:
        fputs("AS_LT:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_LE:
        fputs("AS_LE:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_GT:
        fputs("AS_GT:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_GE:
        fputs("AS_GE:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_ASG:
        fputs("AS_ASG:", stdout);
        astree_show_impl(ast->bin_left);
        astree_show_impl(ast->bin_right);
        break;
    case AS_IF:
        fputs("AS_IF:", stdout);
        astree_show_impl(ast->if_cond);
        astree_show_impl(ast->if_then);
        astree_show_impl(ast->if_else);
        break;
    case AS_WHILE:
        fputs("AS_WHILE:", stdout);
        astree_show_impl(ast->while_cond);
        astree_show_impl(ast->while_body);
        break;
    case AS_FOR:
        fputs("AS_FOR:", stdout);
        astree_show_impl(ast->for_init);
        astree_show_impl(ast->for_cond);
        astree_show_impl(ast->for_step);
        astree_show_impl(ast->for_body);
        break;
    case AS_RET:
        fputs("AS_RET:", stdout);
        astree_show_impl(ast->ret_val);
        break;
    case AS_VAR:
        printf("AS_VAR: '%s'", ast->var_id);
        break;
    case AS_NUM:
        printf("AS_NUM: '%lld'", ast->num_val);
        break;
    default:
        assert(false);
    }
    putchar(')');
    astree_show_impl(ast->next);
    return;
}

void astree_free(astree_t *ast) {
    if (ast == NULL) {
        return;
    }
    switch (ast->kind) {
    case AS_ADD:
    case AS_SUB:
    case AS_MUL:
    case AS_DIV:
    case AS_MOD:
    case AS_EQ:
    case AS_NE:
    case AS_LT:
    case AS_LE:
    case AS_GT:
    case AS_GE:
    case AS_ASG:
        astree_free(ast->bin_left);
        astree_free(ast->bin_right);
        break;
    case AS_IF:
        astree_free(ast->if_cond);
        astree_free(ast->if_then);
        astree_free(ast->if_else);
        break;
    case AS_WHILE:
        astree_free(ast->while_cond);
        astree_free(ast->while_body);
        break;
    case AS_FOR:
        astree_free(ast->for_init);
        astree_free(ast->for_cond);
        astree_free(ast->for_step);
        astree_free(ast->for_body);
        break;
    case AS_RET:
        astree_free(ast->ret_val);
        break;
    case AS_VAR:
    case AS_NUM:
        break;
    default:
        assert(false);
    }
    astree_free(ast->next);
    free(ast);
    return;
}
