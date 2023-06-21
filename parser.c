#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

extern Token *token;
Node *node[256];
Func *func;
Var *local;
int jump = 0;

void prog(void) {
    int i = 0;
    func = calloc(1, sizeof(Func));

    while (!is_eof()) {
        node[i++] = glob();
    }

    node[i] = NULL;
    return;
}

Node *glob(void) {
    local = calloc(1, sizeof(Var));
    Node *nd = node_func();
    Node *arg;
    expect("(");

    while (!consume(")")) {
        arg = node_var();
        arg->next = nd->head;
        nd->head = arg;
        nd->val++;

        if (consume(",")) {
            continue;
        }

        if (consume(")")) {
            break;
        }

        fprintf(stderr, "expected ',' or ')'\n");
        exit(1);
    }

    if (!compare("{")) {
        fprintf(stderr, "expected '{'\n");
        exit(1);
    }

    nd->op1 = stmt();
    nd->ofs = local->ofs;
    return nd;
}

Node *stmt(void) {
    if (compare("int")) {
        Type *ty = new_type();
        char *name = token->str;
        int len = token->len;
        token = token->next;
        Var *var = new_var(name, len, ty);
        expect(";");
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_NOP;
        return nd;
    }

    if (consume("{")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_BLK;
        Node *item = calloc(1, sizeof(Node));
        nd->head = item;

        while (!consume("}")) {
            item->next = stmt();
            item = item->next;
        }

        nd->head = nd->head->next;
        return nd;
    }

    if (consume("if")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_IFEL;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(")");
        nd->op2 = stmt();

        if (consume("else")) {
            nd->op3 = stmt();
        }

        return nd;
    }

    if (consume("while")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_WHILE;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(")");
        nd->op2 = stmt();
        return nd;
    }

    if (consume("for")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_FOR;
        nd->label = jump;
        jump += 2;
        expect("(");
        nd->op1 = expr();
        expect(";");
        nd->op2 = expr();
        expect(";");
        nd->op3 = expr();
        expect(")");
        nd->op4 = stmt();
        return nd;
    }

    if (consume("return")) {
        Node *nd = calloc(1, sizeof(Node));
        nd->kind = ND_RET;
        nd->op1 = expr();
        expect(";");
        return nd;
    }

    Node *nd = expr();
    expect(";");
    return nd;
}

Node *expr(void) {
    return asg();
}

Node *asg(void) {
    Node *nd = equal();

    if (consume("=")) {
        nd = node_res(ND_ASG, nd, asg());
    }

    return nd;
}

Node *equal(void) {
    Node *nd = rel();

    while (true) {
        if (consume("==")) {
            nd = node_res(ND_EQ, nd, rel());
            continue;
        }

        if (consume("!=")) {
            nd = node_res(ND_NE, nd, rel());
            continue;
        }

        return nd;
    }
}

Node *rel(void) {
    Node *nd = add();

    while (true) {
        if (consume("<")) {
            nd = node_res(ND_LT, nd, add());
            continue;
        }

        if (consume("<=")) {
            nd = node_res(ND_LE, nd, add());
            continue;
        }

        if (consume(">")) {
            nd = node_res(ND_LT, add(), nd);
            continue;
        }

        if (consume(">=")) {
            nd = node_res(ND_LE, add(), nd);
            continue;
        }

        return nd;
    }
}

Node *add(void) {
    Node *nd = mul();

    while (true) {
        if (consume("+")) {
            nd = node_res(ND_ADD, nd, mul());
            continue;
        }

        if (consume("-")) {
            nd = node_res(ND_SUB, nd, mul());
            continue;
        }

        return nd;
    }
}

Node *mul(void) {
    Node *nd = unary();

    while (true) {
        if (consume("*")) {
            nd = node_res(ND_MUL, nd, unary());
            continue;
        }

        if (consume("/")) {
            nd = node_res(ND_DIV, nd, unary());
            continue;
        }

        return nd;
    }
}

Node *unary(void) {
    if (consume("&")) {
        return node_res(ND_ADR, unary(), NULL);
    }

    if (consume("*")) {
        return node_res(ND_DER, unary(), NULL);
    }

    if (consume("+")) {
        return node_res(ND_ADD, node_num(0), unary());
    }

    if (consume("-")) {
        return node_res(ND_SUB, node_num(0), unary());
    }

    return prim();
}

Node *prim(void) {
    if (consume("(")) {
        Node *nd = asg();
        expect(")");
        return nd;
    }

    if (token->kind == TK_ID) {
        Node *nd = calloc(1, sizeof(Node));
        nd->name = token->str;
        nd->len = token->len;
        token = token->next;

        if (consume("(")) {
            Func *fn = find_func(nd->name, nd->len);

            if (!fn) {
                fprintf(stderr, "undefined function: %.*s\n", nd->len, nd->name);
                exit(1);
            }

            nd->kind = ND_FNC;
            nd->type = copy_type(fn->type);
            Node *arg;

            while (!consume(")")) {
                arg = expr();
                arg->next = nd->head;
                nd->head = arg;
                nd->val++;

                if (consume(",")) {
                    continue;
                }

                if (consume(")")) {
                    break;
                }

                fprintf(stderr, "expected ',' or ')'\n");
                exit(1);
            }

            return nd;
        }

        Var *var = find_var(nd->name, nd->len);

        if (!var) {
            fprintf(stderr, "undefined variable: %.*s\n", nd->len, nd->name);
            exit(1);
        }

        nd->kind = ND_VAR;
        nd->type = copy_type(var->type);
        nd->ofs = var->ofs;
        return nd;
    }

    return node_num(expect_num());
}

Node *node_res(NodeKind kind, Node *lhs, Node *rhs) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->op1 = lhs;
    nd->op2 = rhs;

    switch (kind) {
        case ND_ASG:
            nd->type = lhs->type;
            break;

        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            if (lhs->type && lhs->type->kind == TY_PTR || rhs->type && rhs->type->kind == TY_PTR) {
                fprintf(stderr, "invalid operands to binary '==', '!=', '<', '<=', '>' or '>='\n");
                exit(1);
            }

            nd->type = lhs->type;
            break;

        case ND_ADD:
        case ND_SUB:
            if (lhs->type && lhs->type->kind == TY_PTR && rhs->type && rhs->type->kind == TY_PTR) {
                fprintf(stderr, "invalid operands to binary '+' or '-'\n");
                exit(1);
            }

            if (lhs->type && lhs->type->kind == TY_PTR) {
                if (lhs->type->ptr->kind == TY_INT) {
                    rhs->size = 4;
                }

                if (lhs->type->ptr->kind == TY_PTR) {
                    rhs->size = 8;
                }

                nd->type = lhs->type;
            }

            if (rhs->type && rhs->type->kind == TY_PTR) {
                if (rhs->type->ptr->kind == TY_INT) {
                    lhs->size = 4;
                }

                if (rhs->type->ptr->kind == TY_PTR) {
                    lhs->size = 8;
                }

                nd->type = rhs->type;
            }

            break;

        case ND_MUL:
        case ND_DIV:
            if (lhs->type && lhs->type->kind == TY_PTR || rhs->type && rhs->type->kind == TY_PTR) {
                fprintf(stderr, "invalid operands to binary '*' or '/'\n");
                exit(1);
            }

            nd->type = lhs->type;
            break;

        case ND_ADR:
            nd->type = calloc(1, sizeof(Type));
            nd->type->kind = TY_PTR;
            nd->type->ptr = lhs->type;
            break;

        case ND_DER:
            if (lhs->type && lhs->type->kind != TY_PTR) {
                fprintf(stderr, "invalid operand to unary '*'\n");
                exit(1);
            }

            nd->type = lhs->type->ptr;
            break;

        default:
            break;
    }

    return nd;
}

Node *node_num(long val) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_NUM;
    nd->type = calloc(1, sizeof(Type));
    nd->type->kind = TY_INT;
    nd->val = val;
    return nd;
}

Node *node_func(void) {
    Type *ty = new_type();
    char *name = token->str;
    int len = token->len;
    token = token->next;
    Func *fn = new_func(name, len, ty);
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_FND;
    nd->name = name;
    nd->len = len;
    nd->type = ty;
    return nd;
}

Node *node_var(void) {
    Type *ty = new_type();
    char *name = token->str;
    int len = token->len;
    token = token->next;
    Var *var = new_var(name, len, ty);
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = ND_VAR;
    nd->type = ty;
    nd->ofs = var->ofs;
    return nd;
}

Func *new_func(char *name, int len, Type *ty) {
    if (find_func(name, len)) {
        fprintf(stderr, "multiple definition of function \'%.*s\'\n", len, name);
        exit(1);
    }

    Func *fn = calloc(1, sizeof(Func));
    fn->type = ty;
    fn->name = name;
    fn->len = len;
    fn->next = func;
    func = fn;
    return fn;
}

Func *find_func(char *name, int len) {
    for (Func *fn = func; fn; fn = fn->next) {
        if (fn->len == len && !strncmp(fn->name, name, len)) {
            return fn;
        }
    }

    return NULL;
}

Var *new_var(char *name, int len, Type *ty) {
    if (find_var(name, len)) {
        fprintf(stderr, "multiple definition of variable \'%.*s\'\n", len, name);
        exit(1);
    }

    Var *var = calloc(1, sizeof(Var));
    var->type = ty;
    var->name = name;
    var->len = len;
    var->ofs = local->ofs + 8;
    var->next = local;
    local = var;
    return var;
}

Var *find_var(char *name, int len) {
    for (Var *var = local; var; var = var->next) {
        if (var->len == len && !strncmp(var->name, name, len)) {
            return var;
        }
    }

    return NULL;
}

Type *new_type(void) {
    Type *ty = calloc(1, sizeof(Type));

    if (consume("int")) {
        ty->kind = TY_INT;
    } else {
        fprintf(stderr, "expected type\n");
        exit(1);
    }

    while (consume("*")) {
        Type *new = calloc(1, sizeof(Type));
        new->kind = TY_PTR;
        new->ptr = ty;
        ty = new;
    }

    return ty;
}

Type *copy_type(Type *ty) {
    Type head;
    Type *cur = &head;

    do {
        cur->ptr = calloc(1, sizeof(Type));
        cur = cur->ptr;
        cur->kind = ty->kind;
        ty = ty->ptr;
    } while (ty);

    return head.ptr;
}
