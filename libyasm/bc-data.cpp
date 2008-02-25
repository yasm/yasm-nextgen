///
/// Data (and LEB128) bytecode
///
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
///
#include "bc_container_util.h"

#include "util.h"

#include "arch.h"
#include "bc_container.h"
#include "bytecode.h"
#include "expr.h"


namespace yasm
{
#if 0
static void
bc_data_finalize(yasm_bytecode *bc, yasm_bytecode *prev_bc)
{
    bytecode_data *bc_data = (bytecode_data *)bc->contents;
    yasm_dataval *dv;
    yasm_intnum *intn;

    /* Convert values from simple expr to value. */
    STAILQ_FOREACH(dv, &bc_data->datahead, link) {
        switch (dv->type) {
            case DV_VALUE:
                if (yasm_value_finalize(&dv->data.val, prev_bc)) {
                    yasm_error_set(YASM_ERROR_TOO_COMPLEX,
                                   N_("data expression too complex"));
                    return;
                }
                break;
            case DV_ULEB128:
            case DV_SLEB128:
                intn = yasm_expr_get_intnum(&dv->data.val.abs, 0);
                if (!intn) {
                    yasm_error_set(YASM_ERROR_NOT_CONSTANT,
                                   N_("LEB128 requires constant values"));
                    return;
                }
                /* Warn for negative values in unsigned environment.
                 * This could be an error instead: the likelihood this is
                 * desired is very low!
                 */
                if (yasm_intnum_sign(intn) == -1 && dv->type == DV_ULEB128)
                    yasm_warn_set(YASM_WARN_GENERAL,
                                  N_("negative value in unsigned LEB128"));
                break;
            default:
                break;
        }
    }
}

static int
bc_data_tobytes(yasm_bytecode *bc, unsigned char **bufp, void *d,
                yasm_output_value_func output_value,
                /*@unused@*/ yasm_output_reloc_func output_reloc)
{
    bytecode_data *bc_data = (bytecode_data *)bc->contents;
    yasm_dataval *dv;
    unsigned char *bufp_orig = *bufp;
    yasm_intnum *intn;
    unsigned int val_len;

    STAILQ_FOREACH(dv, &bc_data->datahead, link) {
        switch (dv->type) {
            case DV_EMPTY:
                break;
            case DV_VALUE:
                val_len = dv->data.val.size/8;
                if (output_value(&dv->data.val, *bufp, val_len,
                                 (unsigned long)(*bufp-bufp_orig), bc, 1, d))
                    return 1;
                *bufp += val_len;
                break;
            case DV_RAW:
                memcpy(*bufp, dv->data.raw.contents, dv->data.raw.len);
                *bufp += dv->data.raw.len;
                break;
            case DV_ULEB128:
            case DV_SLEB128:
                intn = yasm_expr_get_intnum(&dv->data.val.abs, 234);
                if (!intn)
                    yasm_internal_error(N_("non-constant in data_tobytes"));
                *bufp +=
                    yasm_intnum_get_leb128(intn, *bufp, dv->type == DV_SLEB128);
        }
    }

    return 0;
}

yasm_bytecode *
yasm_bc_create_data(yasm_datavalhead *datahead, unsigned int size,
                    int append_zero, yasm_arch *arch, unsigned long line)
{
    bytecode_data *data = yasm_xmalloc(sizeof(bytecode_data));
    yasm_bytecode *bc = yasm_bc_create_common(&bc_data_callback, data, line);
    yasm_dataval *dv, *dv2, *dvo;
    yasm_intnum *intn;
    unsigned long len = 0, rlen, i;


    yasm_dvs_initialize(&data->datahead);

    /* Prescan input data for length, etc.  Careful: this needs to be
     * precisely paired with the second loop.
     */
    STAILQ_FOREACH(dv, datahead, link) {
        switch (dv->type) {
            case DV_EMPTY:
                break;
            case DV_VALUE:
            case DV_ULEB128:
            case DV_SLEB128:
                intn = yasm_expr_get_intnum(&dv->data.val.abs, 0);
                if (intn && dv->type == DV_VALUE && (arch || size == 1))
                    len += size;
                else if (intn && dv->type == DV_ULEB128)
                    len += yasm_intnum_size_leb128(intn, 0);
                else if (intn && dv->type == DV_SLEB128)
                    len += yasm_intnum_size_leb128(intn, 1);
                else {
                    if (len > 0) {
                        /* Create bytecode for all previous len */
                        dvo = yasm_dv_create_raw(yasm_xmalloc(len), len);
                        STAILQ_INSERT_TAIL(&data->datahead, dvo, link);
                        len = 0;
                    }

                    /* Create bytecode for this value */
                    dvo = yasm_xmalloc(sizeof(yasm_dataval));
                    STAILQ_INSERT_TAIL(&data->datahead, dvo, link);
                }
                break;
            case DV_RAW:
                rlen = dv->data.raw.len;
                /* find count, rounding up to nearest multiple of size */
                rlen = (rlen + size - 1) / size;
                len += rlen*size;
                break;
        }
        if (append_zero)
            len++;
    }

    /* Create final dataval for any trailing length */
    if (len > 0) {
        dvo = yasm_dv_create_raw(yasm_xmalloc(len), len);
        STAILQ_INSERT_TAIL(&data->datahead, dvo, link);
    }

    /* Second iteration: copy data and delete input datavals. */
    dv = STAILQ_FIRST(datahead);
    dvo = STAILQ_FIRST(&data->datahead);
    len = 0;
    while (dv && dvo) {
        switch (dv->type) {
            case DV_EMPTY:
                break;
            case DV_VALUE:
            case DV_ULEB128:
            case DV_SLEB128:
                intn = yasm_expr_get_intnum(&dv->data.val.abs, 0);
                if (intn && dv->type == DV_VALUE && (arch || size == 1)) {
                    if (size == 1)
                        yasm_intnum_get_sized(intn,
                                              &dvo->data.raw.contents[len],
                                              1, 8, 0, 0, 1);
                    else
                        yasm_arch_intnum_tobytes(arch, intn,
                                                 &dvo->data.raw.contents[len],
                                                 size, size*8, 0, bc, 1);
                    yasm_value_delete(&dv->data.val);
                    len += size;
                } else if (intn && dv->type == DV_ULEB128) {
                    len += yasm_intnum_get_leb128(intn,
                                                  &dvo->data.raw.contents[len],
                                                  0);
                    yasm_value_delete(&dv->data.val);
                } else if (intn && dv->type == DV_SLEB128) {
                    len += yasm_intnum_get_leb128(intn,
                                                  &dvo->data.raw.contents[len],
                                                  1);
                    yasm_value_delete(&dv->data.val);
                } else {
                    if (len > 0)
                        dvo = STAILQ_NEXT(dvo, link);
                    dvo->type = dv->type;
                    dvo->data.val = dv->data.val;   /* structure copy */
                    dvo->data.val.size = size*8;    /* remember size */
                    dvo = STAILQ_NEXT(dvo, link);
                    len = 0;
                }
                break;
            case DV_RAW:
                rlen = dv->data.raw.len;
                memcpy(&dvo->data.raw.contents[len], dv->data.raw.contents,
                       rlen);
                yasm_xfree(dv->data.raw.contents);
                len += rlen;
                /* pad with 0's to nearest multiple of size */
                rlen %= size;
                if (rlen > 0) {
                    rlen = size-rlen;
                    for (i=0; i<rlen; i++)
                        dvo->data.raw.contents[len++] = 0;
                }
                break;
        }
        if (append_zero)
            dvo->data.raw.contents[len++] = 0;
        dv2 = STAILQ_NEXT(dv, link);
        yasm_xfree(dv);
        dv = dv2;
    }

    return bc;
}

yasm_bytecode *
yasm_bc_create_leb128(yasm_datavalhead *datahead, int sign, unsigned long line)
{
    yasm_dataval *dv;

    /* Convert all values into LEB type, error on strings/raws */
    STAILQ_FOREACH(dv, datahead, link) {
        switch (dv->type) {
            case DV_VALUE:
                dv->type = sign ? DV_SLEB128 : DV_ULEB128;
                break;
            case DV_RAW:
                yasm_error_set(YASM_ERROR_VALUE,
                               N_("LEB128 does not allow string constants"));
                break;
            default:
                break;
        }
    }

    return yasm_bc_create_data(datahead, 0, 0, 0, line);
}

yasm_dataval *
yasm_dv_create_expr(yasm_expr *e)
{
    yasm_dataval *retval = yasm_xmalloc(sizeof(yasm_dataval));

    retval->type = DV_VALUE;
    yasm_value_initialize(&retval->data.val, e, 0);

    return retval;
}

yasm_dataval *
yasm_dv_create_raw(unsigned char *contents, unsigned long len)
{
    yasm_dataval *retval = yasm_xmalloc(sizeof(yasm_dataval));

    retval->type = DV_RAW;
    retval->data.raw.contents = contents;
    retval->data.raw.len = len;

    return retval;
}
#endif
void
append_byte(BytecodeContainer& container, unsigned char val)
{
    Bytecode& bc = container.fresh_bytecode();
    bc.get_fixed().write_8(val);
}

void
append_data(BytecodeContainer& container,
            const IntNum& val,
            unsigned int size,
            const Arch& arch)
{
    Bytecode& bc = container.fresh_bytecode();
    Bytes zero;
    zero.resize(size);
    arch.intnum_tobytes(val, zero, size*8, 0, 1);
    bc.get_fixed().insert(bc.get_fixed().end(), zero.begin(), zero.end());
}

void
append_data(BytecodeContainer& container,
            std::auto_ptr<Expr> expr,
            unsigned int size,
            const Arch& arch)
{
    expr->simplify();
    if (IntNum* intn = expr->get_intnum())
    {
        append_data(container, *intn, size, arch);
        return;
    }
    Bytecode& bc = container.fresh_bytecode();
    bc.append_fixed(size, expr);
}

void
append_data(BytecodeContainer& container,
            const std::string& str,
            bool append_zero)
{
    Bytes& fixed = container.fresh_bytecode().get_fixed();
    fixed.write((const unsigned char *)str.data(), str.length());
    if (append_zero)
        fixed.write_8(0);
}

void
append_data(BytecodeContainer& container,
            const std::string& str,
            unsigned int size,
            bool append_zero)
{
    Bytes& fixed = container.fresh_bytecode().get_fixed();
    std::string::size_type len = str.length();
    fixed.write((const unsigned char *)str.data(), len);
    fixed.write(len % size, 0);
    if (append_zero)
        fixed.write_8(0);
}

} // namespace yasm
