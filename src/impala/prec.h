#ifndef IMPALA_PREC_H
#define IMPALA_PREC_H

#include "impala/token.h"

namespace impala {

enum Prec {
    BOTTOM,
    ASGN,
    COND,
    L_O,
    L_A,
    OR,
    XOR,
    AND,
    EQ,
    REL,
    SHIFT,
    ADD,
    MUL,
    UNARY,
    POSTFIX,
    EVAL,
    TOP,
    NUM_PREC
};

typedef Prec Type2Prec[Token::NUM_TOKENS];

struct BinPrec {
    Prec l;
    Prec r;

    BinPrec() {}
    BinPrec(Prec l, Prec r) : l(l), r(r) {}
};

typedef BinPrec Type2BinPrec[Token::NUM_TOKENS];

struct PrecTable {
    static Type2Prec prefix_r; ///< Right precedence -- for unary prefix operators.
    static Type2Prec infix_l;  ///< Left precedences -- for binary operators.
    static Type2Prec infix_r;  ///< Right precedences -- for binary operators.
    static Type2Prec postfix_l;///< Left precedence -- for unary postfix operators.

private:
    static void init();

    friend void init();
};

} // namespace impala

#endif
