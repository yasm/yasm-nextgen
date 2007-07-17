#include <iostream>

#include <libyasm/intnum.h>
#include <libyasm/expr.h>

using namespace yasm;

int
main()
{
    Expr *e = new Expr(new IntNum(5));
#if 0
    Expr *e2 = new Expr(Expr::NEG, new Expr(new IntNum(5)));
    Expr *e3 = new Expr(e2, Expr::MUL, new Expr(new IntNum(6)));
    Expr *e4 = new Expr(e, Expr::ADD, e3);
#else
    Expr *e2 = new Expr(Expr::NEG, new IntNum(5));
    Expr *e3 = new Expr(e2, Expr::MUL, new IntNum(6));
    Expr *e4 = new Expr(e, Expr::ADD, e3);
#endif

    std::cout << *e4 << std::endl;

    e4->simplify(false);

    std::cout << *e4 << std::endl;

    delete e4;

    return 0;
}
