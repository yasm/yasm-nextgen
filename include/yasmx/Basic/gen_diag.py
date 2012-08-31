# !/usr/bin/env python
# Diagnostic include file generation
#
#  Copyright (C) 2009  Peter Johnson
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
import sys

scriptname = "gen_diag.py"

def lprint(*args, **kwargs):
    sep = kwargs.pop("sep", ' ')
    end = kwargs.pop("end", '\n')
    file = kwargs.pop("file", sys.stdout)
    file.write(sep.join(args))
    file.write(end)

groups = dict()
class Group(object):
    def __init__(self, name, subgroups=None):
        self.name = name
        if subgroups is None:
            self.subgroups = []
        else:
            self.subgroups = subgroups
        self.members = []

def add_group(name, subgroups=None):
    if name in groups:
        lprint("Warning: duplicate group %s" % name, file=sys.stderr)
    groups[name] = Group(name, subgroups)

categories = dict()
class Category(object):
    def __init__(self, name):
        self.name = name

def add_category(name):
    if name in categories:
        lprint("Warning: duplicate category %s" % name, file=sys.stderr)
    categories[name] = Category(name)

class Diag(object):
    def __init__(self, name, cls, desc, mapping=None, group="", category=""):
        self.name = name
        self.cls = cls
        self.desc = desc
        self.mapping = mapping
        self.group = group
        if category not in categories:
            add_category(category)
        self.category = category

diags = dict()
def add_diag(name, cls, desc, mapping=None, group="", category=""):
    if name in diags:
        lprint("Warning: duplicate diag %s" % name, file=sys.stderr)
    diags[name] = Diag(name, cls, desc, mapping, group, category)

def add_warning(name, desc, mapping=None, group="", category=""):
    if group:
        if group not in groups:
            lprint("Unrecognized warning group %s" % group, file=sys.stderr)
        else:
            groups[group].members.append(name)
    add_diag(name, "WARNING", desc, mapping, group, category)

def add_fatal(name, desc, category=""):
    add_diag(name, "ERROR", desc, mapping="FATAL", category=category)

def add_error(name, desc, mapping=None, category=""):
    add_diag(name, "ERROR", desc, mapping, category=category)

def add_note(name, desc, category=""):
    add_diag(name, "NOTE", desc, mapping="FATAL", category=category)

def output_diag_kinds(f):
    lprint("#ifndef YASM_DIAGNOSTICKINDS_H", file=f)
    lprint("#define YASM_DIAGNOSTICKINDS_H", file=f)
    lprint("namespace yasm { namespace diag {", file=f)
    lprint("enum {", file=f)
    for name in sorted(diags):
        diag = diags[name]
        lprint("%s," % diag.name, file=f)
    lprint("NUM_BUILTIN_DIAGNOSTICS", file=f)
    lprint("};", file=f)
    lprint("}}", file=f)
    lprint("#endif", file=f)

def output_diags(f):
    for name in sorted(diags):
        diag = diags[name]
        lprint("{ diag::%s, diag::MAP_%s, CLASS_%s, %d, \"%s\", %s }," % (
            diag.name,
            diag.mapping or diag.cls,
            diag.cls,
            categories[diag.category].index,
            diag.desc.replace("\\", "\\\\").replace('"', '\\"'),
            diag.group and "\"%s\"" % diag.group or "0"), file=f)

def output_groups(f):
    # enumerate all groups and set indexes first
    for n, name in enumerate(sorted(groups)):
        groups[name].index = n

    # output arrays
    for name in sorted(groups):
        group = groups[name]
        if group.members:
            lprint("static const short DiagArray%d[] = {" % group.index,
                   file=f, end='')
            for member in group.members:
                lprint("diag::%s," % member, file=f, end='')
            lprint("-1 };", file=f)
        if group.subgroups:
            lprint("static const char DiagSubGroup%d[] = { " % group.index,
                   file=f, end='')
            for subgroup in group.subgroups:
                lprint("%d, " % groups[subgroup].index, file=f, end='')
            lprint("-1 };", file=f)

    # output table
    lprint("static const WarningOption OptionTable[] = {", file=f)
    for name in sorted(groups):
        group = groups[name]
        lprint("  { \"%s\", %s, %s }," % (
                group.name,
                group.members and ("DiagArray%d" % group.index) or "0",
                group.subgroups and ("DiagSubGroup%d" % group.index) or "0"),
               file=f)
    lprint("};", file=f)

def output_categories(f):
    # enumerate all categories and set indexes first
    for n, name in enumerate(sorted(categories)):
        categories[name].index = n

    # output table
    lprint("static const char *CategoryNameTable[] = {", file=f)
    for name in sorted(categories):
        category = categories[name]
        lprint("  \"%s\"," % category.name, file=f)
    lprint("  \"<<END>>\"", file=f)
    lprint("};", file=f)

#####################################################################
# Groups (for command line -W option)
#####################################################################

add_group("comment")
add_group("unrecognized-char")
add_group("orphan-labels")
add_group("init-nobits")
add_group("uninit-contents")
add_group("size-override")
add_group("signed-overflow")

#####################################################################
# Diagnostics
#####################################################################

add_fatal("fatal_too_many_errors", "too many errors emitted, stopping now")

# Frontend
add_fatal("fatal_module_load", "could not load %0 '%1'")
add_fatal("fatal_module_combo", "'%1' is not a valid %0 for %2 '%3'")
add_fatal("fatal_objfmt_machine_mismatch",
          "object format '%0' does not support architecture '%1' machine '%2'")
add_warning("warn_unknown_warning_option", "unknown warning option '%0'")
add_fatal("fatal_file_open", "could not open file '%0'")
add_fatal("fatal_standard_modules", "could not load standard modules")
add_warning("warn_plugin_load", "could not load plugin '%0'")
add_fatal("fatal_no_input_files", "no input files specified")
add_fatal("fatal_unrecognized_module", "unrecognized %0 '%1'")
add_warning("warn_unknown_command_line_option",
            "unknown command line argument '%0'; try '-help'")
add_fatal("fatal_bad_defsym",
          "bad defsym '%0'; format is --defsym name=value")

# Source manager
add_fatal("err_cannot_open_file", "cannot open file '%0': %1")
add_fatal("err_file_modified",
          "file '%0' modified since it was first processed")
add_fatal("err_unsupported_bom", "%0 byte order mark detected in '%1', but "
          "encoding is not supported")

# yobjdump
add_error("err_file_open", "could not open file '%0'")
add_error("err_unrecognized_object_format", "unrecognized object format '%0'")
add_error("err_unrecognized_object_file",
          "file is not recognized as a '%0' object file")
add_error("err_unrecognized_file_format", "file format not recognized")

add_note("note_previous_definition", "previous definition is here")
# note_matching - this is used as a continuation of a previous diagnostic,
# e.g. to specify the '(' when we expected a ')'.
add_note("note_matching", "to match this '%0'")

# Generic lex/parse errors
add_error("err_parse_error", "parse error")
add_error("err_expected_ident", "expected identifier")
add_error("err_expected_ident_lparen", "expected identifier or '('")
add_error("err_expected_lparen", "expected '('")
add_error("err_expected_rparen", "expected ')'")
add_error("err_expected_lsquare", "expected '['")
add_error("err_expected_rsquare", "expected ']'")
add_error("err_expected_greater", "expected '>'")
add_error("err_expected_lparen_after", "expected '(' after '%0'")
add_error("err_expected_lparen_after_id", "expected '(' after %0")
add_error("err_expected_plus", "expected '+'")
add_error("err_expected_comma", "expected ','")
add_error("err_expected_colon", "expected ':'")
add_error("err_expected_at", "expected '@'")
add_error("err_expected_colon_after_segreg", "expected ':' after segment register")
add_error("err_expected_operand", "expected operand")
add_error("err_expected_memory_address", "expected memory address")
add_error("err_expected_expression", "expected expression")
add_error("err_expected_expression_after", "expected expression after '%0'")
add_error("err_expected_expression_after_id", "expected expression after %0")
add_error("err_expected_expression_or_string",
          "expression or string expected")
add_error("err_expected_string", "expected string")
add_error("err_expected_integer", "expected integer")
add_error("err_expected_float", "expected float")
add_error("err_expected_filename", "expected filename")
add_error("err_eol_junk", "junk at end of line")

# Unrecognized errors/warnings
add_error("err_unrecognized_value", "unrecognized value")
add_error("err_unrecognized_register", "unrecognized register name")
add_error("err_unrecognized_instruction", "unrecognized instruction")
add_error("err_unrecognized_directive", "unrecognized directive")
add_warning("warn_unrecognized_directive", "unrecognized directive")
add_warning("warn_unrecognized_ident", "unrecognized identifier")

# NASM errors
add_error("err_non_reserve_in_absolute_section",
          "only RES* allowed within absolute section")

# GAS warnings
add_warning("warn_scale_without_index",
            "scale factor without an index register")
add_error("err_align_no_alignment", ".ALIGN directive must specify alignment")
add_error("err_comm_size_expected", "size expected for .COMM")
add_error("err_fill_size_not_absolute", "size must be an absolute expression")
add_error("err_bad_register_name", "bad register name")
add_error("err_bad_register_index", "bad register index")
add_error("err_missing_or_invalid_immediate",
          "missing or invalid immediate expression")
add_error("err_rept_without_endr", ".rept without matching .endr")
add_error("err_endr_without_rept", ".endr without matching .rept")
add_error("err_bad_argument_to_syntax_dir", "bad argument to syntax directive")
add_warning("warn_popsection_without_pushsection",
            ".popsection without corresponding .pushsection; ignored")
add_warning("warn_previous_without_section",
            ".previous without corresponding .section; ignored")
add_warning("warn_zero_assumed_for_missing_expression",
            "zero assumed for missing expression")
add_error("err_bad_memory_operand", "bad memory operand")

# Value
add_error("err_too_complex_expression", "expression too complex")
add_error("err_value_not_constant", "value not constant")
add_error("err_too_complex_jump", "jump target expression too complex")
add_error("err_invalid_jump_target", "invalid jump target")
add_error("err_float_expression_too_complex",
          "floating point expression too complex")

# Expression
add_error("err_data_value_register", "data values cannot have registers")
add_error("err_expr_contains_float",
          "expression must not contain floating point value")

# Integer/float calculation
add_error("err_divide_by_zero", "divide by zero")
add_error("err_float_invalid_op", "invalid floating point operation")
add_error("err_int_invalid_op", "invalid integer operation")
add_warning("warn_float_underflow", "underflow in floating point expression")
add_warning("warn_float_overflow", "overflow in floating point expression")
add_warning("warn_float_inexact", "inexact floating point result")
add_error("err_invalid_op_use", "invalid use of operator '%0'")

# Integer/float output
add_warning("warn_signed_overflow",
            "value does not fit in signed %0 bit field",
            group="signed-overflow")
add_warning("warn_unsigned_overflow", "value does not fit in %0 bit field")
add_warning("warn_truncated", "misaligned value, truncating to %0 bit boundary")

# Effective address
add_error("err_invalid_ea_segment", "invalid segment in effective address")

# Immediate
add_error("err_imm_segment_override",
          "cannot have segment override on immediate")

# TIMES
add_error("err_expected_expression_after_times",
          "expected expression after TIMES")
add_error("err_expected_insn_after_times",
          "instruction expected after TIMES expression")

# INCBIN
add_error("err_incbin_expected_filename",
          "filename string expected after INCBIN")
add_error("err_incbin_expected_start_expression",
          "expression expected for INCBIN start")
add_error("err_incbin_expected_length_expression",
          "expression expected for INCBIN maximum length")

# Label/instruction expected
add_error("err_expected_insn_after_label",
          "instruction expected after label")
add_error("err_expected_insn_or_label_after_eol",
          "label or instruction expected at start of line")

# File
add_error("err_file_read", "unable to read file '%0': %1")
add_error("err_file_output_seek", "unable to seek on output file",
          mapping="FATAL")
add_error("err_file_output_position",
          "could not get file position on output file",
          mapping="FATAL")

# Align
add_error("err_align_not_integer", "alignment constraint is not an integer")
add_error("err_align_boundary_not_const", "align boundary must be a constant")
add_error("err_align_fill_not_const", "align fill must be a constant")
add_error("err_align_skip_not_const", "align maximum skip must be a constant")
add_error("err_align_code_not_found",
          "could not find an appropriate code alignment size")
add_error("err_align_invalid_code_size", "invalid code alignment size %0")

# Incbin
add_error("err_incbin_start_too_complex", "start expression too complex")
add_error("err_incbin_start_not_absolute", "start expression not absolute")
add_error("err_incbin_start_not_const", "start expression not constant")
add_error("err_incbin_maxlen_too_complex",
          "maximum length expression too complex")
add_error("err_incbin_maxlen_not_absolute",
          "maximum length expression not absolute")
add_error("err_incbin_maxlen_not_const",
          "maximum length expression not constant")
add_warning("warn_incbin_start_after_eof", "start past end of file")

# LEB128
add_error("err_leb128_too_complex", "LEB128 value is relative or too complex")
add_warning("warn_negative_uleb128", "negative value in unsigned LEB128")

# Multiple
add_error("err_multiple_too_complex", "multiple expression too complex")
add_error("err_multiple_not_absolute", "multiple expression not absolute")
add_error("err_multiple_setpos",
          "cannot combine multiples and setting assembly position")
add_error("err_multiple_negative", "multiple cannot be negative")
add_error("err_multiple_unknown", "could not determine multiple")

# ORG
add_error("err_org_overlap", "ORG overlap with already existing data")
add_error("err_org_start_not_const", "org start must be a constant")
add_error("err_org_fill_not_const", "org fill must be a constant")

# Symbol
add_error("err_symbol_undefined", "undefined symbol '%0' (first use)")
add_note("note_symbol_undefined_once",
         "(Each undefined symbol is reported only once.)")
add_error("err_symbol_redefined", "redefinition of '%0'")
add_warning("warn_extern_defined", "'%0' both defined and declared extern")
add_note("note_extern_declaration", "'%0' declared extern here")
add_error("err_equ_circular_reference", "circular equ reference detected")

# Insn
add_error("err_equ_circular_reference_mem",
          "circular reference detected in memory expression")
add_error("err_equ_circular_reference_imm",
          "circular reference detected in immediate expression")
add_error("err_too_many_operands", "too many operands, maximum of %0")
add_warning("warn_prefixes_skipped", "skipping prefixes on this instruction")
add_error("err_bad_num_operands", "invalid number of operands")
add_error("err_bad_operand_size", "invalid operand size")
add_error("err_requires_cpu", "requires CPU%0")
add_warning("warn_insn_with_cpu", "identifier is an instruction with CPU%0")
add_error("err_bad_insn_operands",
          "invalid combination of opcode and operands")
add_error("err_missing_jump_form", "no %0 form of that jump instruction exists")
add_warning("warn_multiple_seg_override",
            "multiple segment overrides, using leftmost")

# x86 Insn
add_warning("warn_indirect_call_no_deref", "indirect call without '*'")
add_warning("warn_skip_prefixes", "skipping prefixes on this instruction")
add_error("err_16addr_64mode", "16-bit addresses not supported in 64-bit mode")
add_error("err_bad_address_size", "unsupported address size")
add_error("err_dest_not_src1_or_src3",
          "one of source operand 1 (%0) or 3 (%1) must match dest operand")
add_warning("warn_insn_in_64mode",
            "identifier is an instruction in 64-bit mode")
add_error("err_insn_invalid_64mode", "instruction not valid in 64-bit mode")
add_error("err_data32_override_64mode",
          "cannot override data size to 32 bits in 64-bit mode")
add_error("err_addr16_override_64mode",
          "cannot override address size to 16 bits in 64-bit mode")
add_warning("warn_prefix_in_64mode", "identifier is a prefix in 64-bit mode")
add_warning("warn_reg_in_xxmode", "identifier is a register in %0-bit mode")
add_warning("warn_seg_ignored_in_xxmode",
            "'%0' segment register ignored in %1-bit mode")
add_warning("warn_multiple_lock_rep",
            "multiple LOCK or REP prefixes, using leftmost")
add_warning("warn_multiple_acq_rel",
            "multiple XACQUIRE/XRELEASE prefixes, using leftmost")
add_warning("warn_ignore_rex_on_jump", "ignoring REX prefix on jump")
add_warning("warn_illegal_rex_insn",
            "REX prefix not allowed on this instruction, ignoring")
add_warning("warn_rex_overrides_internal", "overriding generated REX prefix")
add_warning("warn_multiple_rex", "multiple REX prefixes, using leftmost")
add_error("err_high8_rex_conflict",
          "cannot use A/B/C/DH with instruction needing REX")
add_warning("warn_address_size_ignored", "address size override ignored")

# Immediate
add_error("err_imm_too_complex", "immediate expression too complex")

# EffAddr
add_error("err_invalid_ea", "invalid effective address")
add_error("err_ea_too_complex", "effective address too complex")

# x86 EffAddr
add_warning("warn_fixed_invalid_disp_size", "invalid displacement size; fixed")
add_error("err_invalid_disp_size",
          "invalid effective address (displacement size)")
add_error("err_64bit_ea_not64mode",
          "invalid effective address (64-bit in non-64-bit mode)")
add_warning("warn_rip_rel_not64mode",
            "RIP-relative directive ignored in non-64-bit mode")
add_error("err_16bit_ea_64mode",
          "16-bit addresses not supported in 64-bit mode")
add_error("err_ea_length_unknown",
          "indeterminate effective address during length calculation")

# Optimizer
add_error("err_optimizer_circular_reference", "circular reference detected")
add_error("err_optimizer_secondary_expansion",
          "secondary expansion of an external/complex value")

# Output
add_error("err_too_many_relocs", "too many relocations in section '%0'")
add_error("err_reloc_contains_float", "cannot relocate float")
add_error("err_reloc_too_complex", "relocation too complex")
add_error("err_reloc_invalid_size", "invalid relocation size")
add_error("err_wrt_not_supported", "WRT not supported")
add_error("err_wrt_too_complex", "WRT expression too complex")
add_error("err_wrt_across_sections", "cannot WRT across sections")
add_error("err_invalid_wrt", "invalid WRT")
add_error("err_common_size_too_complex", "common size too complex")
add_error("err_common_size_negative", "common size cannot be negative")

# Label
add_note("note_duplicate_label_prev", "previous label defined here")
add_warning("warn_orphan_label",
            "label alone on a line without a colon might be in error",
            group="orphan-labels")
add_warning("warn_no_nonlocal", "no preceding non-local label")

# Directive
add_error("err_expected_directive_name", "expected directive name")
add_error("err_invalid_directive_argument", "invalid argument to directive")
add_error("err_directive_no_args", "directive requires an argument")
add_error("err_directive_too_few_args", "directive requires %0 arguments")
add_warning("warn_directive_one_arg", "directive only uses first argument")
add_error("err_float_in_directive",
          "directive argument cannot be floating point")
add_error("err_value_id", "value must be an identifier")
add_error("err_value_integer", "value must be an integer")
add_error("err_value_expression", "value must be an expression")
add_error("err_value_string", "value must be a string")
add_error("err_value_string_or_id",
          "value must be a string or an identifier")
add_error("err_value_power2", "value is not a power of 2")
add_error("err_value_register", "value must be a register")
add_warning("warn_unrecognized_qualifier", "unrecognized qualifier")

# Size/offset
add_error("err_no_size", "no size specified")
add_error("err_size_expression", "size must be an expression")
add_error("err_size_integer", "size specifier must be an integer")
add_error("err_no_offset", "no offset specified")
add_error("err_offset_expression", "offset must be an expression")

# Section directive
add_warning("warn_section_redef_flags",
            "section flags ignored on section redeclaration")
add_error("err_expected_flag_string", "expected flag string")
add_warning("warn_unrecognized_section_attribute",
            "unrecognized section attribute: '%0'")

# Operand size override
add_error("err_register_size_override", "cannot override register size")
add_warning("warn_operand_size_override",
            "overridding operand size from %0-bit to %1-bit",
            group="size-override")
add_warning("warn_operand_size_duplicate",
            "duplicate '%0' operand size override",
            group="size-override")

# Operand
add_error("err_offset_expected_after_colon", "offset expected after ':'")

# Memory address
add_error("err_colon_required_after_segreg",
          "':' required after segment register")

# Lexer 
add_warning("null_in_string",
            "null character(s) preserved in string literal")
add_warning("null_in_char",
            "null character(s) preserved in character literal")
add_warning("null_in_file", "null character ignored",
            group="unrecognized-char")
add_warning("backslash_newline_space",
            "backslash and newline separated by space")
add_warning("warn_unrecognized_char", "ignoring unrecognized character '%0'",
            group="unrecognized-char")

add_warning("warn_multi_line_eol_comment", "multi-line end-of-line comment",
            group="comment")
add_warning("warn_nested_block_comment", "'/*' within block comment",
            group="comment")
add_warning("escaped_newline_block_comment_end",
            "escaped newline between */ characters at block comment end",
            group="comment")

add_warning("warn_unterminated_string",
            "unterminated string; newline inserted")
add_error("err_unterminated_string", "missing terminating %0 character")
add_error("err_unterminated_block_comment", "unterminated /* comment")

# Literals
add_warning("warn_integer_too_large", "integer constant is too large")
add_warning("warn_integer_too_large_for_signed",
            "integer constant is so large that it is unsigned")
add_warning("warn_unknown_escape", "unknown escape sequence '\\%0'")
add_warning("warn_oct_escape_out_of_range",
            "octal escape sequence out of range")
add_error("err_hex_escape_no_digits",
          "\\x used with no following hex digits")
add_error("err_invalid_decimal_digit",
          "invalid digit '%0' in decimal constant")
add_error("err_invalid_binary_digit",
          "invalid digit '%0' in binary constant")
add_error("err_invalid_octal_digit",
          "invalid digit '%0' in octal constant")
add_error("err_invalid_suffix_integer_constant",
          "invalid suffix '%0' on integer constant")
add_error("err_invalid_suffix_float_constant",
          "invalid suffix '%0' on floating constant")
add_error("err_exponent_has_no_digits", "exponent has no digits")

add_warning("warn_expected_hex_digit", "expected hex digit after \\x")
add_error("err_unicode_escape_requires_hex",
          "expected hex digit for Unicode character code (%0 digits required)")

# Preprocessor
add_error("err_pp_file_not_found", "'%0' file not found", mapping="FATAL")
add_error("err_pp_error_opening_file", "error opening file '%0': %1",
          mapping="FATAL")
add_error("err_pp_empty_filename", "empty filename")
add_error("err_pp_include_too_deep", "include nested too deeply")
add_error("err_pp_else_after_else", "else after else")
add_error("err_pp_elseif_after_else", "elseif after else")
add_error("err_pp_else_without_if", "else without if")
add_error("err_pp_elseif_without_if", "elseif without if")
add_error("err_pp_endif_without_if", "endif without if")
add_error("err_pp_if_without_endif", "if without endif")
add_error("err_pp_cond_not_constant", "non-constant conditional expression")

add_error("err_pp_expected_line", "expected %%line")
add_fatal("fatal_pp_errors", "preprocessor errors")

# Output
add_warning("warn_nobits_data",
            "initialized space declared in nobits section: ignoring",
            group="init-nobits")
add_warning("warn_uninit_zero",
            "uninitialized space declared in code/data section: zeroing",
            group="uninit-contents")
add_warning("warn_export_equ",
            "object format does not support exporting EQU/absolute values")
add_warning("warn_name_too_long", "name too long, truncating to %0 bytes")
add_error("err_equ_not_integer", "EQU value not an integer expression")
add_error("err_equ_too_complex", "EQU value too complex")
add_warning("warn_equ_undef_ref",
            "EQU value contains undefined symbol; not emitting")
add_error("err_common_size_not_integer",
          "COMMON data size not an integer expression")

# DWARF debug format
add_error("err_loc_file_number_missing", "file number required")
add_error("err_loc_file_number_not_integer", "file number is not an integer")
add_error("err_loc_file_number_invalid", "file number less than one")
add_error("err_loc_line_number_missing", "line number required")
add_error("err_loc_line_number_not_integer", "line number is not an integer")
add_error("err_loc_column_number_not_integer",
          "column number is not an integer")
add_error("err_loc_must_be_in_section",
          "[loc] can only be used inside of a section")
add_error("err_loc_is_stmt_not_zero_or_one", "is_stmt value is not 0 or 1")
add_error("err_loc_isa_not_integer", "isa value is not an integer")
add_error("err_loc_isa_less_than_zero", "isa value is less than 0")
add_error("err_loc_discriminator_not_integer",
          "discriminator value is not an integer")
add_error("err_loc_discriminator_less_than_zero",
          "discriminator value is less than 0")
add_warning("warn_unrecognized_loc_option", "unrecognized loc option '%0'")
add_warning("warn_unrecognized_numeric_qualifier",
            "unrecognized numeric qualifier")
add_error("err_loc_option_requires_value", "option %0 requires value")

add_error("err_loc_missing_filename", "file number given but no filename")

add_error("err_file_number_unassigned", "file number %0 unassigned")

# Object reading
add_error("err_object_read_not_supported",
          "object format does not support reading")
add_error("err_object_header_unreadable", "could not read object header")
add_error("err_not_file_type", "not an %0 file")
add_error("err_no_section_table", "no section table")
add_error("err_no_symbol_string_table", "could not find symbol string table")
add_error("err_multiple_symbol_tables", "only one symbol table supported")
add_error("err_string_table_unreadable", "could not read string table data")
add_error("err_section_header_too_small", "section header too small")
add_error("err_section_data_unreadable", "could not read section '%0' data")
add_error("err_section_relocs_unreadable", "could not read section '%0' relocs")
add_error("err_symbol_unreadable", "could not read symbol table entry")
add_error("err_symbol_entity_size_zero", "symbol table entity size is zero")
add_error("err_invalid_string_offset", "invalid string table offset")

# Binary object format
add_error("err_org_too_complex", "ORG expression is too complex")
add_error("err_org_negative", "ORG expression is negative")
add_error("err_start_too_complex", "start expression is too complex")
add_error("err_vstart_too_complex", "vstart expression is too complex")
add_error("err_start_too_large", "section '%0' start value is too large")
add_error("err_indeterminate_section_length",
          "could not determine length of section '%0'")
add_error("err_section_follows_unknown",
          "section '%0' follows an invalid or unknown section '%1'")
add_error("err_section_follows_loop",
          "follows loop between section '%0' and section '%1'")
add_error("err_section_vfollows_unknown",
          "section '%0' vfollows an invalid or unknown section '%1'")
add_error("err_section_vfollows_loop",
          "vfollows loop between section '%0' and section '%1'")
add_error("err_section_overlap",
          "sections '%0' and '%1' overlap by %2 bytes")
add_error("err_section_before_origin",
          "section '%0' starts before origin (ORG)")
add_warning("warn_section_align_larger",
            "section '%0' internal align of %1 is greater than '%2' of %3; using '%2'")
add_warning("warn_start_not_aligned",
            "start inconsistent with align; using aligned value")
add_error("err_vstart_not_aligned", "vstart inconsistent with valign")
add_warning("warn_cannot_open_map_file", "cannot open map file '%0': %1")
add_error("err_bin_extern_ref",
          "binary object format does not support external references")
add_warning("warn_bin_unsupported_decl",
            "binary object format does not support %0 variables")

# ELF object format
add_warning("warn_unrecognized_symbol_type", "unrecognized symbol type '%0'")
add_warning("warn_multiple_symbol_visibility",
            "more than one symbol visibility provided; using last")
add_error("err_expected_merge_entity_size",
          "entity size for SHF_MERGE not specified")
add_error("err_expected_group_name",
          "group name for SHF_GROUP not specified")

# ELF/DWARF CFI
add_error("err_nested_cfi",
          "previous CFI entry not closed (missing .cfi_endproc")
add_warning("warn_outside_cfiproc",
            "CFI instruction used without previous .cfi_startproc; ignored")
add_warning("warn_cfi_endproc_before_startproc",
            ".cfi_endproc without corresponding .cfi_startproc; ignored")
add_error("err_eof_inside_cfiproc",
          "open CFI at end of file; missing .cfi_endproc directive")
add_error("err_cfi_state_stack_empty",
          "CFI state restore without previous remember")
add_warning("warn_cfi_routine_ignored", "personality routine ignored")
add_error("err_cfi_invalid_encoding", "invalid or unsupported encoding")
add_error("err_cfi_routine_required", "personality routine symbol required")

# COFF object format
add_warning("warn_coff_section_name_length",
            "COFF section names limited to 8 characters: truncating")
add_warning("warn_coff_no_readonly_sections",
            "standard COFF does not support read-only data sections")
add_warning("warn_nested_def",
            ".def pseudo-op used inside of .def/.endef; ignored")
add_warning("warn_outside_def",
            "%0 pseudo-op used outside of .def/.endef; ignored")
add_warning("warn_endef_before_def",
            ".endef pseudo-op used before .def; ignored")

# Mach-O object format
add_error("err_macho_segment_section_required",
          ".section directive requires both segment and section names")
add_warning("warn_macho_segment_name_length",
            "Mach-O segment names limited to 16 characters: truncating")
add_warning("warn_macho_section_name_length",
            "Mach-O section names limited to 16 characters: truncating")
add_error("err_macho_unknown_section_type", "unrecognized section type")
add_error("err_macho_unknown_section_attr", "unrecognized section attribute")
add_error("err_macho_align_too_big",
          "Mach-O does not support alignments > 16384")
add_error("err_macho_no_32_absolute_reloc_in_64",
          'Mach-O cannot apply 32 bit absolute relocations in 64 bit mode; consider "[_symbol wrt rip]" for mem access, "qword" and "dq _foo" for pointers.')
add_error("err_macho_desc_requires_expr", ".desc requires n_desc expression")

# Win32 object format
add_error("err_win32_align_too_big",
          "Win32 does not support alignments > 8192")

# Win64 object format
add_error("err_eof_proc_frame", "end of file in procedure frame")
add_note("note_proc_started_here", "procedure started here")
add_error("err_negative_offset", "offset cannot be negative")
add_error("err_offset_not_multiple", "offset of %0 is not a multiple of %1")
add_error("err_offset_out_of_range",
          "offset of %0 bytes, must be between %1 and %2")
add_error("err_prologue_too_large", "prologue %0 bytes, must be <256")
add_error("err_too_many_unwind_codes", "%0 unwind codes, maximum of 255")
add_note("note_prologue_end", "prologue ended here")

# RDF object format
add_error("err_segment_requires_type",
          "new segment declared without type code")
add_error("err_invalid_bss_record", "BSS record is not 4 bytes long")
add_error("err_refseg_out_of_range", "relocation refseg %0 out of range")
add_error("err_invalid_refseg", "invalid refseg %0")

# XDF object format
add_error("err_xdf_headers_unreadable", "could not read XDF header tables")
add_error("err_xdf_common_unsupported",
          "XDF object format does not support COMMON variables")

#####################################################################
# Output generation
#####################################################################

if __name__ == "__main__":
    if len(sys.argv) != 5:
        lprint("Usage: gen_diag.py <DiagnosticGroups.cpp>", file=sys.stderr)
        lprint("    <DiagnosticCategories.cpp>", file=sys.stderr)
        lprint("    <DiagnosticKinds.h> <StaticDiagInfo.inc>", file=sys.stderr)
        sys.exit(2)
    output_groups(open(sys.argv[1], "wt"))
    output_categories(open(sys.argv[2], "wt"))
    output_diag_kinds(open(sys.argv[3], "wt"))
    output_diags(open(sys.argv[4], "wt"))
