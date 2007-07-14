#include <iostream>

#include <libyasm/intnum.h>
#include <libyasm/expr.h>

using namespace yasm;

int
main()
{
    Expr *e = new Expr(new IntNum(5));
#if 0
    Expr *e2 = new Expr(EXPR_NEG, new Expr(new IntNum(5)));
    Expr *e3 = new Expr(e2, EXPR_MUL, new Expr(new IntNum(6)));
    Expr *e4 = new Expr(e, EXPR_ADD, e3);
#else
    Expr *e2 = new Expr(EXPR_NEG, new IntNum(5));
    Expr *e3 = new Expr(e2, EXPR_MUL, new IntNum(6));
    Expr *e4 = new Expr(e, EXPR_ADD, e3);
#endif

    std::cout << *e4 << std::endl;

    e4->simplify(false);

    std::cout << *e4 << std::endl;

    delete e4;

    return 0;
}
