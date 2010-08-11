#! /usr/bin/env python
#
# epsgeom - extract geometry from a EPS file.
#
# Translated from Perl to Python by Peter Johnson, 2010
#
# Copyright (C) 2004 The FreeBSD Project. All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
import sys, os, re

x = None
y = None
width = None
height = None

if len(sys.argv) != 5:
    sys.exit("Error: invalid arguments.")

mode = sys.argv[1]
hres = int(sys.argv[2])
vres = int(sys.argv[3])
fn = sys.argv[4]

realfn = os.path.realpath(fn)

f = open(realfn)
for line in f:
    m = re.match(r"%%BoundingBox:\s+([-\d]+)\s+([-\d]+)\s+([-\d]+)\s+([-\d]+)", line)
    if m:
        x = int(m.group(1))
        y = int(m.group(2))
        width = int(m.group(3))
        height = int(m.group(4))
        break
    #print >>sys.stderr, "DEBUG:", line
f.close()

if x is None:
    sys.exit("Error: no BoundingBox found.")

width -= x
height -= y

if mode == "-replace":
    replace = {'@X@': x,
               '@Y@': y,
               '@MX@': -x,
               '@MY@': -y,
               '@WIDTH@': width,
               '@MWIDTH@': -width,
               '@HEIGHT@': height,
               '@MHEIGHT@': -height,
               '@ANGLE@': 0,
               '@INPUT@': realfn}

    for k, v in replace.items():
	print "-e s,%s,%s,g " % (k, v)
elif mode == "-offset":
    #print >>sys.stderr, "DEBUG: %d, %d" % (-x, y)
    print "<< /PageOffset [%d %d] >> setpagedevice" % (-x, y)
    f = open(realfn)
    for line in f:
        print line
    f.close()
    print
    print "showpage"
elif mode == "-geom":
    #print >>sys.stderr, "DEBUG:", x, y, width, height

    # (int)(((double)hres * (double)width  / 72.0) + 0.5),
    gx = int(hres * width / 72.0 + 0.5)
    # (int)(((double)vres * (double)height / 72.0) + 0.5),
    gy = int(vres * height / 72.0 + 0.5)
    print "%dx%d" % (gx, gy)
else:
    exit("Error: invalid mode specified.")

