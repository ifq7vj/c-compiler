#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"

void generator(FILE *, astree_t *);
static void generator_expr(FILE *, astree_t *);
static void emit_global(FILE *, const char *);
static void emit_label(FILE *, const char *);
static void emit_push(FILE *, const char *);
static void emit_pop(FILE *, const char *);
static void emit_add(FILE *, const char *, const char *);
static void emit_sub(FILE *, const char *, const char *);
static void emit_mul(FILE *, const char *);
static void emit_div(FILE *, const char *);
static void emit_push_num(FILE *, long long);
static void emit_ret(FILE *);

void generator(FILE *ofp, astree_t *ast) {
    emit_global(ofp, "main");
    emit_label(ofp, "main");
    generator_expr(ofp, ast);
    emit_pop(ofp, "%rax");
    emit_ret(ofp);
    return;
}

void generator_expr(FILE *ofp, astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        generator_expr(ofp, ast->lhs);
        generator_expr(ofp, ast->rhs);
        emit_pop(ofp, "%rbx");
        emit_pop(ofp, "%rax");
        emit_add(ofp, "%rbx", "%rax");
        emit_push(ofp, "%rax");
        break;
    case AS_SUB:
        generator_expr(ofp, ast->lhs);
        generator_expr(ofp, ast->rhs);
        emit_pop(ofp, "%rbx");
        emit_pop(ofp, "%rax");
        emit_sub(ofp, "%rbx", "%rax");
        emit_push(ofp, "%rax");
        break;
    case AS_MUL:
        generator_expr(ofp, ast->lhs);
        generator_expr(ofp, ast->rhs);
        emit_pop(ofp, "%rbx");
        emit_pop(ofp, "%rax");
        emit_mul(ofp, "%rbx");
        emit_push(ofp, "%rax");
        break;
    case AS_DIV:
        generator_expr(ofp, ast->lhs);
        generator_expr(ofp, ast->rhs);
        emit_pop(ofp, "%rbx");
        emit_pop(ofp, "%rax");
        emit_div(ofp, "%rbx");
        emit_push(ofp, "%rax");
        break;
    case AS_MOD:
        generator_expr(ofp, ast->lhs);
        generator_expr(ofp, ast->rhs);
        emit_pop(ofp, "%rbx");
        emit_pop(ofp, "%rax");
        emit_div(ofp, "%rbx");
        emit_push(ofp, "%rdx");
        break;
    case AS_NUM:
        emit_push_num(ofp, ast->num);
        break;
    default:
        assert(false);
    }
    return;
}

void emit_global(FILE *ofp, const char *label) {
    fprintf(ofp, ".globl %s\n", label);
    return;
}

void emit_label(FILE *ofp, const char *label) {
    fprintf(ofp, "%s:\n", label);
    return;
}

void emit_push(FILE *ofp, const char *reg) {
    fprintf(ofp, "    pushq %s\n", reg);
    return;
}

void emit_pop(FILE *ofp, const char *reg) {
    fprintf(ofp, "    popq %s\n", reg);
    return;
}

void emit_add(FILE *ofp, const char *lhs, const char *rhs) {
    fprintf(ofp, "    addq %s, %s\n", lhs, rhs);
    return;
}

void emit_sub(FILE *ofp, const char *lhs, const char *rhs) {
    fprintf(ofp, "    subq %s, %s\n", lhs, rhs);
    return;
}

void emit_mul(FILE *ofp, const char *rhs) {
    fprintf(ofp, "    imulq %s\n", rhs);
    return;
}

void emit_div(FILE *ofp, const char *rhs) {
    fprintf(ofp, "    cqto\n");
    fprintf(ofp, "    idivq %s\n", rhs);
    return;
}

void emit_push_num(FILE *ofp, long long num) {
    fprintf(ofp, "    pushq $%lld\n", num);
    return;
}

void emit_ret(FILE *ofp) {
    fprintf(ofp, "    ret\n");
    return;
}
