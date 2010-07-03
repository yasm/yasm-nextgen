#!/usr/bin/env python
# Generate array-of-const-string from text file.
#
#  Copyright (C) 2006-2007  Peter Johnson
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
import sys

def lprint(*args, **kwargs):
    sep = kwargs.pop("sep", ' ')
    end = kwargs.pop("end", '\n')
    file = kwargs.pop("file", sys.stdout)
    file.write(sep.join(args))
    file.write(end)

def file_to_string(fout, str_name, fin_name):
    from os.path import basename
    lprint("// This file auto-generated from %s by genstring.py - don't edit it\n" % basename(fin_name), file=fout)
    lprint("static const char* %s[] = {" % str_name, file=fout)
    lprint("\n".join('    "%s",' %
                     l.strip().replace('\\', '\\\\').replace('"', '\\"')
                     for l in open(fin_name)), file=fout)
    lprint("};", file=fout)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        lprint("Usage: genstring.py <string> <outfile> <file>", file=sys.stderr)
        sys.exit(2)
    file_to_string(open(sys.argv[2], "w"), sys.argv[1], sys.argv[3])
