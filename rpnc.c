#include "mpc.h"

#include <editline/history.h>
#include <editline/readline.h>

typedef struct
{
    int type;
    long num;
    int err;
} rpn_val;

enum
{
    RVAL_NUM,
    RVAL_ERR
};

enum
{
    RERR_DIV_ZERO,
    RERR_BAD_OP,
    RERR_BAD_NUM
};

rpn_val rpn_val_num(long x)
{
    rpn_val v;
    v.type = RVAL_NUM;
    v.num = x;
    return v;
}

rpn_val rpn_val_err(int x)
{
    rpn_val v;
    v.type = RVAL_ERR;
    v.err = x;
    return v;
}

void rpn_val_print(rpn_val v)
{
    switch (v.type)
    {
        case RVAL_NUM:
            printf("%li\n", v.num);
            break;
        case RVAL_ERR:
            if (v.err == RERR_DIV_ZERO)
            {
                printf("Error: Division by zero.\n");
            }
            else if (v.err == RERR_BAD_OP)
            {
                printf("Error: Invalid operator.\n");
            }
            else if (v.err == RERR_BAD_NUM)
            {
                printf("Error: Invalid number.\n");
            }
            break;
    }
}

rpn_val eval_op(rpn_val x, char *op, rpn_val y)
{
    if (x.type == RVAL_ERR)
    {
        return x;
    }

    if (y.type == RVAL_ERR)
    {
        return y;
    }

    if (strcmp(op, "+") == 0)
    {
        return rpn_val_num(x.num + y.num);
    }

    if (strcmp(op, "-") == 0)
    {
        return rpn_val_num(x.num - y.num);
    }

    if (strcmp(op, "*") == 0)
    {
        return rpn_val_num(x.num * y.num);
    }

    if (strcmp(op, "/") == 0)
    {
        return (y.num == 0) ? rpn_val_err(RERR_DIV_ZERO) : rpn_val_num(x.num / y.num);
    }

    return rpn_val_err(RERR_BAD_OP);
}


rpn_val eval(mpc_ast_t *t)
{
    if (strstr(t->tag, "number"))
    {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? rpn_val_num(x) : rpn_val_err(RERR_BAD_NUM);
    }

    char *op = t->children[1]->contents;
    rpn_val x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr"))
    {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char **argv)
{
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Rpnc = mpc_new("rpnc");

    mpca_lang(
        MPCA_LANG_DEFAULT,
        "                                                 \
            number : /-?[0-9]+/ ;                         \
            operator : '+' | '-' | '*' | '/' ;            \
            expr : <number> | '(' <operator> <expr>+ ')' ; \
            rpnc : /^/ <operator> <expr>+ /$/ ;          \
        ",
        Number, Operator, Expr, Rpnc
    );

    printf("Reverse Polish Notation Calculator\nPress CTRL+C to exit.\n");

    while (1)
    {
        char *input = readline("rpnc> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Rpnc, &r))
        {
            rpn_val result = eval(r.output);
            rpn_val_print(result);
            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Rpnc);

    return 0;
}

