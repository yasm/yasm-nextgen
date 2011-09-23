/* eval.h   header file for eval.c
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 */

#ifndef NASM_EVAL_H
#define NASM_EVAL_H

#include "nasm.h"

namespace yasm { class Expr; class Object; }

namespace nasm {

extern yasm::Object* yasm_object;

/*
 * The evaluator itself.
 */
yasm::Expr *nasm_evaluate (scanner sc, void *scprivate, struct tokenval *tv,
                          int critical, efunc report_error);

} // namespace nasm

#endif
