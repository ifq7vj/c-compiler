#include <stdlib.h>
#include <stdio.h>

#include "main.h"

const char *reg_arg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

extern Node *node[256];

void gen_code(void) {
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    for (int i = 0; node[i]; i++) {
        gen_func(node[i]);
    }

    return;
}

void gen_func(Node *nd) {
    if (nd->kind == ND_NOP) {
        return;
    }

    printf("\n%.*s:\n", nd->len, nd->name);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", nd->ofs);

    for (int i = 0; i < nd->val; i++) {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", (i + 1) * 8);

        if (i < 6) {
            printf("    mov [rax], %s\n", reg_arg[i]);
        } else {
            printf("    mov rdi, rbp\n");
            printf("    add rdi, %d\n", (i - 4) * 8);
            printf("    mov rdi, [rdi]\n");
            printf("    mov [rax], rdi\n");
        }
    }

    for (Node *cur = nd->head; cur; ) {
        Node *del = cur;
        cur = cur->next;
        gen_stmt(del);
    }

    gen_stmt(nd->op1);
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
}

void gen_stmt(Node *nd) {
    switch (nd->kind) {
        case ND_NOP:
            break;

        case ND_BLK:
            for (Node *cur = nd->head; cur; cur = cur->next) {
                gen_stmt(cur);
            }

            break;

        case ND_IFEL:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label);
            gen_stmt(nd->op2);
            printf("    jmp .L%d\n", nd->label + 1);
            printf("\n.L%d:\n", nd->label);

            if (nd->op3) {
                gen_stmt(nd->op3);
            }

            printf("\n.L%d:\n", nd->label + 1);
            break;

        case ND_WHILE:
            printf("\n.L%d:\n", nd->label);
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label + 1);
            gen_stmt(nd->op2);
            printf("    jmp .L%d\n", nd->label);
            printf("\n.L%d:\n", nd->label + 1);
            break;

        case ND_FOR:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("\n.L%d:\n", nd->label);
            gen_expr(nd->op2);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", nd->label + 1);
            gen_stmt(nd->op4);
            gen_expr(nd->op3);
            printf("    pop rax\n");
            printf("    jmp .L%d\n", nd->label);
            printf("\n.L%d:\n", nd->label + 1);
            break;

        case ND_RET:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            break;

        default:
            gen_expr(nd);
            printf("    pop rax\n");
            break;
    }

    return;
}

void gen_expr(Node *nd) {
    switch (nd->kind) {
        case ND_ADR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->op1->ofs);
            printf("    push rax\n");
            break;

        case ND_DER:
            gen_expr(nd->op1);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;

        case ND_ASG:
            gen_lval(nd->op1);
            gen_expr(nd->op2);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            break;

        case ND_NUM:
            printf("    push %ld\n", nd->val * (nd->size? nd->size: 1));
            break;

        case ND_FNC:
            printf("    push rsp\n");
            printf("    push [rsp]\n");

            if (nd->val > 6 && nd->val % 2 == 0) {
                printf("    add rsp, 8\n");
                printf("    and rsp, -16\n");
            } else {
                printf("    and rsp, -16\n");
                printf("    add rsp, 8\n");
            }

            for (Node *cur = nd->head; cur; cur = cur->next) {
                gen_expr(cur);
            }

            for (int i = 0; i < nd->val && i < 6; i++) {
                printf("    pop %s\n", reg_arg[i]);
            }

            printf("    call %.*s\n", nd->len, nd->name);

            for (int i = 6; i < nd->val; i++) {
                printf("    pop rdi\n");
            }

            printf("    mov rsp, [rsp]\n");
            printf("    push rax\n");
            break;

        case ND_VAR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->ofs);
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;

        default:
            gen_bin(nd);
            break;
    }

    return;
}

void gen_bin(Node *nd) {
    gen_expr(nd->op1);
    gen_expr(nd->op2);
    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (nd->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;

        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;

        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;

        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;

        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;

        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;

        default:
            break;
    }

    printf("    push rax\n");
    return;
}

void gen_lval(Node *nd) {
    switch (nd->kind) {
        case ND_VAR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", nd->ofs);
            printf("    push rax\n");
            break;

        case ND_DER:
            gen_lval(nd->op1);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            break;

        default:
            fprintf(stderr, "invalid lvalue\n");
            exit(1);
    }

    return;
}
