#include <iostream>

#include <libyasmx/intnum.h>
#include <libyasmx/expr.h>

using namespace yasm;

int
main()
{
    Expr::Ptr e(new Expr(IntNum(5)));
#if 0
    Expr *e2 = new Expr(Expr::NEG, IntNum(5));
    Expr *e3 = new Expr(e2, Expr::MUL, IntNum(6));
    Expr *e4 = new Expr(e, Expr::ADD, e3);
#else
    Expr::Ptr e2(new Expr(Op::NEG, IntNum(5)));
    Expr::Ptr e3(new Expr(e2, Op::MUL, IntNum(6)));
    Expr::Ptr e4(new Expr(e, Op::ADD, e3));
    Expr e5(*e4);
#endif

    std::cout << *e4 << std::endl;

    e4->simplify();

    std::cout << *e4 << std::endl;

    std::cout << e5 << std::endl;
    e5 = *e4;
    std::cout << e5 << std::endl;

#if 0
    delete e4;
#endif

    return 0;
}
