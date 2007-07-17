#include <iostream>

#include <libyasm/intnum.h>
#include <libyasm/expr.h>

using namespace yasm;

int
main()
{
    std::auto_ptr<Expr> e(new Expr(new IntNum(5)));
#if 0
    Expr *e2 = new Expr(Expr::NEG, new IntNum(5));
    Expr *e3 = new Expr(e2, Expr::MUL, new IntNum(6));
    Expr *e4 = new Expr(e, Expr::ADD, e3);
#else
    std::auto_ptr<Expr> e2(new Expr(Expr::NEG, new IntNum(5)));
    std::auto_ptr<Expr> e3(new Expr(e2, Expr::MUL, new IntNum(6)));
    std::auto_ptr<Expr> e4(new Expr(e, Expr::ADD, e3));
#endif

    std::cout << *e4 << std::endl;

    e4->simplify(false);

    std::cout << *e4 << std::endl;

#if 0
    delete e4;
#endif

    return 0;
}
