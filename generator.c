#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"

void generator(FILE *, astree_t *);
static void generate_prog_x86_64(FILE *, astree_t *);
static void generate_expr_x86_64(FILE *, astree_t *);
static void generate_prog_aarch64(FILE *, astree_t *);
static void generate_expr_aarch64(FILE *, astree_t *);

void generator(FILE *ofp, astree_t *ast) {
#ifdef __x86_64__
    generate_prog_x86_64(ofp, ast);
#elif __aarch64__
    generate_prog_aarch64(ofp, ast);
#else
    assert(false);
#endif
    return;
}

void generate_prog_x86_64(FILE *ofp, astree_t *ast) {
    fputs(".global main\n", ofp);
    fputs("main:\n", ofp);
    generate_expr_x86_64(ofp, ast);
    fputs("    popq %rax\n", ofp);
    fputs("    ret\n", ofp);
    return;
}

void generate_expr_x86_64(FILE *ofp, astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        generate_expr_x86_64(ofp, ast->lhs);
        generate_expr_x86_64(ofp, ast->rhs);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    addq %rbx, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_SUB:
        generate_expr_x86_64(ofp, ast->lhs);
        generate_expr_x86_64(ofp, ast->rhs);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    subq %rbx, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_MUL:
        generate_expr_x86_64(ofp, ast->lhs);
        generate_expr_x86_64(ofp, ast->rhs);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    imulq %rbx\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_DIV:
        generate_expr_x86_64(ofp, ast->lhs);
        generate_expr_x86_64(ofp, ast->rhs);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cqto\n", ofp);
        fputs("    idivq %rbx\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_MOD:
        generate_expr_x86_64(ofp, ast->lhs);
        generate_expr_x86_64(ofp, ast->rhs);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cqto\n", ofp);
        fputs("    idivq %rbx\n", ofp);
        fputs("    pushq %rdx\n", ofp);
        break;
    case AS_NUM:
        fprintf(ofp, "    pushq $%lld\n", ast->num);
        break;
    default:
        assert(false);
    }
    return;
}

void generate_prog_aarch64(FILE *ofp, astree_t *ast) {
    fputs(".global main\n", ofp);
    fputs("main:\n", ofp);
    generate_expr_aarch64(ofp, ast);
    fputs("    ldr x0, [sp], #16\n", ofp);
    fputs("    ret\n", ofp);
    return;
}

void generate_expr_aarch64(FILE *ofp, astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        generate_expr_aarch64(ofp, ast->lhs);
        generate_expr_aarch64(ofp, ast->rhs);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    add x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_SUB:
        generate_expr_aarch64(ofp, ast->lhs);
        generate_expr_aarch64(ofp, ast->rhs);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sub x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_MUL:
        generate_expr_aarch64(ofp, ast->lhs);
        generate_expr_aarch64(ofp, ast->rhs);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    mul x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_DIV:
        generate_expr_aarch64(ofp, ast->lhs);
        generate_expr_aarch64(ofp, ast->rhs);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sdiv x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_MOD:
        generate_expr_aarch64(ofp, ast->lhs);
        generate_expr_aarch64(ofp, ast->rhs);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sdiv x2, x0, x1\n", ofp);
        fputs("    msub x0, x1, x2, x0\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_NUM:
        fprintf(ofp, "    mov x0, #%lld\n", ast->num);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    default:
        assert(false);
    }
    return;
}
