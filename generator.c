#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"

void generator(FILE *, astree_t *);
static void generate_prog(FILE *, astree_t *);
static void generate_stmt(FILE *, astree_t *);
static void generate_expr(FILE *, astree_t *);

void generator(FILE *ofp, astree_t *ast) {
    generate_prog(ofp, ast);
    return;
}

#ifdef __x86_64__
void generate_prog(FILE *ofp, astree_t *ast) {
    fputs(".global main\n", ofp);
    fputs("main:\n", ofp);
    fputs("    pushq %rbp\n", ofp);
    fputs("    movq %rsp, %rbp\n", ofp);
    fputs("    subq $256, %rsp\n", ofp);
    generate_stmt(ofp, ast);
    fputs("    movq %rbp, %rsp\n", ofp);
    fputs("    popq %rbp\n", ofp);
    fputs("    ret\n", ofp);
    return;
}

void generate_stmt(FILE *ofp, astree_t *ast) {
    if (ast == NULL) {
        return;
    }
    switch (ast->kind) {
    case AS_BLK:
        generate_stmt(ofp, ast->blk_body);
        generate_stmt(ofp, ast->blk_next);
        break;
    case AS_IF:
        generate_expr(ofp, ast->if_cond);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq $0, %rax\n", ofp);
        fprintf(ofp, "    je .Lelse%zu\n", ast->if_jmp);
        generate_stmt(ofp, ast->if_then);
        fprintf(ofp, "    jmp .Lend%zu\n", ast->if_jmp);
        fprintf(ofp, ".Lelse%zu:\n", ast->if_jmp);
        generate_stmt(ofp, ast->if_else);
        fprintf(ofp, ".Lend%zu:\n", ast->if_jmp);
        break;
    case AS_WHILE:
        fprintf(ofp, ".Lbegin%zu:\n", ast->while_jmp);
        generate_expr(ofp, ast->while_cond);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq $0, %rax\n", ofp);
        fprintf(ofp, "    je .Lend%zu\n", ast->while_jmp);
        generate_stmt(ofp, ast->while_body);
        fprintf(ofp, "    jmp .Lbegin%zu\n", ast->while_jmp);
        fprintf(ofp, ".Lend%zu:\n", ast->while_jmp);
        break;
    case AS_FOR:
        generate_expr(ofp, ast->for_init);
        fprintf(ofp, ".Lbegin%zu:\n", ast->for_jmp);
        generate_expr(ofp, ast->for_cond);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq $0, %rax\n", ofp);
        fprintf(ofp, "    je .Lend%zu\n", ast->for_jmp);
        generate_stmt(ofp, ast->for_body);
        generate_expr(ofp, ast->for_step);
        fprintf(ofp, "    jmp .Lbegin%zu\n", ast->for_jmp);
        fprintf(ofp, ".Lend%zu:\n", ast->for_jmp);
        break;
    case AS_RET:
        generate_expr(ofp, ast->bin_left);
        fputs("    popq %rax\n", ofp);
        fputs("    movq %rbp, %rsp\n", ofp);
        fputs("    popq %rbp\n", ofp);
        fputs("    ret\n", ofp);
        break;
    default:
        generate_expr(ofp, ast);
        fputs("    popq %rax\n", ofp);
        break;
    }
    return;
}

void generate_expr(FILE *ofp, astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    addq %rbx, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_SUB:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    subq %rbx, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_MUL:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    imulq %rbx\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_DIV:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cqto\n", ofp);
        fputs("    idivq %rbx\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_MOD:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cqto\n", ofp);
        fputs("    idivq %rbx\n", ofp);
        fputs("    pushq %rdx\n", ofp);
        break;
    case AS_EQ:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    sete %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_NE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    setne %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_LT:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    setl %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_LE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    setle %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_GT:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    setg %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_GE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rbx\n", ofp);
        fputs("    popq %rax\n", ofp);
        fputs("    cmpq %rbx, %rax\n", ofp);
        fputs("    setge %al\n", ofp);
        fputs("    movzbq %al, %rax\n", ofp);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_ASG:
        generate_expr(ofp, ast->bin_right);
        fputs("    popq %rax\n", ofp);
        fprintf(ofp, "    movq %%rax, -%zu(%%rbp)\n", ast->bin_left->var_ofs << 3);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_VAR:
        fprintf(ofp, "    movq -%zu(%%rbp), %%rax\n", ast->var_ofs << 3);
        fputs("    pushq %rax\n", ofp);
        break;
    case AS_NUM:
        fprintf(ofp, "    pushq $%lld\n", ast->num_val);
        break;
    default:
        assert(false);
    }
    return;
}
#elif __aarch64__
void generate_prog(FILE *ofp, astree_t *ast) {
    fputs(".global main\n", ofp);
    fputs("main:\n", ofp);
    fputs("    str x29, [sp, #-16]!\n", ofp);
    fputs("    mov x29, sp\n", ofp);
    fputs("    sub sp, sp, #256\n", ofp);
    generate_stmt(ofp, ast);
    fputs("    mov sp, x29\n", ofp);
    fputs("    ldr x29, [sp], #16\n", ofp);
    fputs("    ret\n", ofp);
    return;
}

void generate_stmt(FILE *ofp, astree_t *ast) {
    if (ast == NULL) {
        return;
    }
    switch (ast->kind) {
    case AS_BLK:
        generate_stmt(ofp, ast->blk_body);
        generate_stmt(ofp, ast->blk_next);
        break;
    case AS_IF:
        generate_expr(ofp, ast->if_cond);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, #0\n", ofp);
        fprintf(ofp, "    beq .Lelse%zu\n", ast->if_jmp);
        generate_stmt(ofp, ast->if_then);
        fprintf(ofp, "    b .Lend%zu\n", ast->if_jmp);
        fprintf(ofp, ".Lelse%zu:\n", ast->if_jmp);
        generate_stmt(ofp, ast->if_else);
        fprintf(ofp, ".Lend%zu:\n", ast->if_jmp);
        break;
    case AS_WHILE:
        fprintf(ofp, ".Lbegin%zu:\n", ast->while_jmp);
        generate_expr(ofp, ast->while_cond);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, #0\n", ofp);
        fprintf(ofp, "    beq .Lend%zu\n", ast->while_jmp);
        generate_stmt(ofp, ast->while_body);
        fprintf(ofp, "    b .Lbegin%zu\n", ast->while_jmp);
        fprintf(ofp, ".Lend%zu:\n", ast->while_jmp);
        break;
    case AS_FOR:
        generate_expr(ofp, ast->for_init);
        fprintf(ofp, ".Lbegin%zu:\n", ast->for_jmp);
        generate_expr(ofp, ast->for_cond);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, #0\n", ofp);
        fprintf(ofp, "    beq .Lend%zu\n", ast->for_jmp);
        generate_stmt(ofp, ast->for_body);
        generate_expr(ofp, ast->for_step);
        fprintf(ofp, "    b .Lbegin%zu\n", ast->for_jmp);
        fprintf(ofp, ".Lend%zu:\n", ast->for_jmp);
        break;
    case AS_RET:
        generate_expr(ofp, ast->bin_left);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    mov sp, x29\n", ofp);
        fputs("    ldr x29, [sp], #16\n", ofp);
        fputs("    ret\n", ofp);
        break;
    default:
        generate_expr(ofp, ast);
        fputs("    ldr x0, [sp], #16\n", ofp);
        break;
    }
    return;
}

void generate_expr(FILE *ofp, astree_t *ast) {
    switch (ast->kind) {
    case AS_ADD:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    add x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_SUB:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sub x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_MUL:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    mul x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_DIV:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sdiv x0, x0, x1\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_MOD:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    sdiv x2, x0, x1\n", ofp);
        fputs("    msub x0, x1, x2, x0\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_EQ:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, eq\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_NE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, ne\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_LT:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, lt\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_LE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, le\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_GT:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, gt\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_GE:
        generate_expr(ofp, ast->bin_left);
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x1, [sp], #16\n", ofp);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fputs("    cmp x0, x1\n", ofp);
        fputs("    cset x0, ge\n", ofp);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_ASG:
        generate_expr(ofp, ast->bin_right);
        fputs("    ldr x0, [sp], #16\n", ofp);
        fprintf(ofp, "    str x0, [x29, #-%zu]\n", ast->bin_left->var_ofs << 4);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_VAR:
        fprintf(ofp, "    ldr x0, [x29, #-%zu]\n", ast->var_ofs << 4);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    case AS_NUM:
        fprintf(ofp, "    mov x0, #%lld\n", ast->num_val);
        fputs("    str x0, [sp, #-16]!\n", ofp);
        break;
    default:
        assert(false);
    }
    return;
}
#else
#error
#endif
