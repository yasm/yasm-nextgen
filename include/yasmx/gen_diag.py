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
scriptname = "gen_diag.py"

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
        print "Warning: duplicate group %s" % name
    groups[name] = Group(name, subgroups)

class Diag(object):
    def __init__(self, name, cls, desc, mapping=None, group=""):
        self.name = name
        self.cls = cls
        self.desc = desc
        self.mapping = mapping
        self.group = group

diags = dict()
def add_diag(name, cls, desc, mapping=None, group=""):
    if name in diags:
        print "Warning: duplicate diag %s" % name
    diags[name] = Diag(name, cls, desc, mapping, group)

def add_warning(name, desc, mapping=None, group=""):
    if group:
        if group not in groups:
            print "Unrecognized warning group %s" % group
        else:
            groups[group].members.append(name)
    add_diag(name, "WARNING", desc, mapping, group)

def add_error(name, desc, mapping=None):
    add_diag(name, "ERROR", desc, mapping)

def add_note(name, desc):
    add_diag(name, "NOTE", desc, mapping="FATAL")

def output_diags(f):
    for name in sorted(diags):
        diag = diags[name]
        print >>f, "DIAG(%s, CLASS_%s, diag::MAP_%s, \"%s\", %s)" % (
            diag.name,
            diag.cls,
            diag.mapping or diag.cls,
            diag.desc.encode("string_escape"),
            diag.group and "\"%s\"" % diag.group or "0")

def output_groups(f):
    # enumerate all groups and set indexes first
    for n, name in enumerate(sorted(groups)):
        groups[name].index = n

    # output arrays
    print >>f, "#ifdef GET_DIAG_ARRAYS"
    for name in sorted(groups):
        group = groups[name]
        if group.members:
            print >>f, "static const short DiagArray%d[] = {" % group.index,
            for member in group.members:
                print >>f, "diag::%s," % member,
            print >>f, "-1 };"
        if group.subgroups:
            print >>f, "static const char DiagSubGroup%d[] = { " \
                % group.index,
            for subgroup in group.subgroups:
                print >>f, "%d, " % groups[subgroup].index,
            print >>f, "-1 };"

    print >>f, "#endif // GET_DIAG_ARRAYS\n"

    # output table
    print >>f, "#ifdef GET_DIAG_TABLE"
    for name in sorted(groups):
        group = groups[name]
        print >>f, "  { \"%s\", %s, %s }," % (
            group.name,
            group.members and ("DiagArray%d" % group.index) or "0",
            group.subgroups and ("DiagSubGroup%d" % group.index) or "0")
    print >>f, "#endif // GET_DIAG_TABLE\n"

#####################################################################
# Groups (for command line -W option)
#####################################################################

add_group("comment")
add_group("unrecognized-char")
add_group("orphan-labels")
add_group("uninit-contents")
add_group("size-override")

#####################################################################
# Diagnostics
#####################################################################

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
add_error("err_expected_lparen_after", "expected '(' after '%0'")
add_error("err_expected_lparen_after_id", "expected '(' after %0")
add_error("err_expected_plus", "expected '+'")
add_error("err_expected_comma", "expected ','")
add_error("err_expected_colon", "expected ':'")
add_error("err_expected_colon_after_segreg", "expected ':' after segment register")
add_error("err_expected_operand", "expected operand")
add_error("err_expected_memory_address", "expected memory address")
add_error("err_expected_expression", "expected expression")
add_error("err_expected_expression_after", "expected expression after '%0'")
add_error("err_expected_expression_after_id", "expected expression after %0")
add_error("err_expected_expression_or_string",
          "expression or string expected")
add_error("err_expected_string", "expected string")
add_error("err_expected_integer", "expected integer");
add_error("err_expected_filename", "expected filename");
add_error("err_eol_junk", "junk at end of line")

# Unrecognized errors/warnings
add_error("err_unrecognized_value", "unrecognized value")
add_error("err_unrecognized_register", "unrecognized register name")
add_error("err_unrecognized_instruction", "unrecognized instruction")
add_warning("warn_unrecognized_directive", "unrecognized directive")
add_warning("warn_unrecognized_ident", "unrecognized identifier")

# GAS warnings
add_warning("warn_scale_without_index",
            "scale factor without an index register")

# Expression
add_error("err_data_value_register", "data values cannot have registers")

# Label/instruction expected
add_error("err_expected_insn_after_label",
          "instruction expected after label")
add_error("err_expected_insn_after_times",
          "instruction expected after TIMES expression")
add_error("err_expected_insn_label_after_eol",
          "label or instruction expected at start of line")

# Label
add_note("note_duplicate_label_prev", "previous label defined here")
add_warning("warn_orphan_label",
            "label alone on a line without a colon might be in error",
            group="orphan-labels")
add_warning("warn_no_nonlocal", "no preceding non-local label")

# Directive
add_error("err_expected_directive_name", "expected directive name")
add_error("err_invalid_directive_argument", "invalid argument to directive")

# Operand size override
add_error("err_register_size_override", "cannot override register size")
add_warning("warn_operand_size_override",
            "overridding operand size from %0-bit to %1-bit",
            group="size-override")
add_warning("warn_operand_size_duplicate",
            "duplicate '%0' operand size override",
            group="size-override")

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

add_error("err_unterminated_string", "missing terminating %0 character")

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

#####################################################################
# Output generation
#####################################################################

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print >>sys.stderr, "Usage: gen_diag.py <DiagnosticGroups.inc>"
        print >>sys.stderr, "    <DiagnosticKinds.inc>"
        sys.exit(2)
    output_groups(file(sys.argv[1], "wt"))
    output_diags(file(sys.argv[2], "wt"))
