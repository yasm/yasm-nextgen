#! /usr/bin/env python
# Regression test runner
#
#  Copyright (C) 2010  Peter Johnson
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
import os, time, subprocess

yasmexe = None
outdir = None

def path_splitall(path):
    outpath = []
    while True:
        (head, tail) = os.path.split(path)
        if not head:
            break
        path = head
        outpath.append(tail)
    outpath.append(path)
    outpath.reverse()
    return outpath

def run_test(name, fullpath):
    # We default to bin output and parser based on extension
    parser = name.endswith(".s") and "gas" or "nasm"
    commentsep = (parser == "gas") and "#" or ";"
    yasmargs = ["yasm", "-f", "bin", "-p", parser]

    # Read the input file in its entirety.  We use this for various things.
    with open(fullpath) as f:
        inputfile = f.read()
    inputlines = inputfile.splitlines()

    # Read the first line of the file for test-specific options.

    # Expected failure: "[fail]"
    expectfail = "[fail]" in inputlines[0]

    # command line override: "yasm <args>"
    customarg = inputlines[0].find("yasm")
    if customarg != -1:
        import shlex
        yasmargs = shlex.split(inputlines[0][customarg:].rstrip())

    # Notify start of test
    print "[ RUN      ] %s (%s)%s" % (name, " ".join(yasmargs[1:]), expectfail and "{fail}" or "")

    # Specify the output filename as we pipe the input.
    outpath = os.path.splitext("_".join(path_splitall(name)))[0] + ".out"
    outfullpath = os.path.join(outdir, outpath)
    yasmargs.extend(["-o", outfullpath])

    # We pipe the input, so append "-" to the command line for stdin input.
    yasmargs.append("-")

    # Run yasm!
    start = time.time()
    ok = False
    proc = subprocess.Popen(yasmargs, bufsize=4096, executable=yasmexe,
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    (stdoutdata, stderrdata) = proc.communicate(inputfile)
    if proc.returncode == 0 and not expectfail:
        ok = True
    elif proc.returncode < 0:
        print " CRASHED: received signal %d from yasm" % (-proc.returncode)
    elif expectfail:
        ok = True
    end = time.time()

    # Check results
    if ok:
        # Check error/warnings output.
        # If there's a .ew file, use it.
        golden = []
        try:
            with open(os.path.splitext(fullpath)[0] + ".ew") as f:
                golden = [l.rstrip() for l in f.readlines()]
        except IOError:
            pass

        result = [l for l in stderrdata.splitlines() if l.startswith("<stdin>:")]

        resultfn = os.path.splitext(outpath)[0] + ".ew"
        mismatch = False
        if len(golden) != len(result):
            print "%s: error/warning mismatches" % resultfn
            mismatch = True
        for i, (o, g) in enumerate(zip(result, golden)):
            if o != g:
                print "%s:%d: mismatch on error/warning line %d" % (resultfn, i)
                print " Expected: %s" % g
                print " Actual: %s" % o
                mismatch = True
        if mismatch:
            ok = False
            # Save stderr output
            with open(resultfn, "w") as f:
                f.write(stderrdata)

        if not expectfail:
            # Check output file.
            # If there's a .hex file, use it; otherwise scan the input file
            # for comments starting with "out:" followed by hex digits.
            golden = []
            try:
                with open(os.path.splitext(fullpath)[0] + ".hex") as f:
                    golden = [l.strip() for l in f.readlines()]
            except IOError:
                for l in inputlines:
                    comment = l.partition(commentsep)[2].strip()
                    if not comment.startswith('out:'):
                        continue
                    golden.extend(comment[4:].split())
            golden = [int(x, 16) for x in golden if x]

            goldenfn = os.path.splitext(outpath)[0] + ".golden"

            # check result file
            with open(outfullpath, "rb") as f:
                result = f.read()
            mismatch = False
            if len(golden) != len(result):
                print "%s: output length %d (expected %d)" % (outpath, len(result), len(golden))
                mismatch = True
            for i, (o, g) in enumerate(zip([ord(x) for x in result], golden)):
                if o != g:
                    print "%s:%d: mismatch: %s (expected %s)" % (outpath, i, hex(o), hex(g))
                    print "  (only the first mismatch is reported)"
                    mismatch = True
                    break

            if mismatch:
                ok = False
                # save golden version to binary file
                print "Expected binary file: %s" % goldenfn
                with open(goldenfn, "wb") as f:
                    f.write("".join([chr(x) for x in golden]))

    # Summarize test result
    if ok:
        result = "      OK"
    else:
        result = " FAILED "
    print "[ %s ] %s (%d ms)" % (result, name, int((end-start)*1000))
    return ok

def run_all(bpath):
    ntests = 0
    ndirs = 0
    npassed = 0
    failed = []
    print "[==========] Running tests."
    start = time.time()
    for root, dirs, files in os.walk(bpath):
        somefiles = [f for f in files if f.endswith(".asm") or f.endswith(".s")]
        if not somefiles:
            continue
        ndirs += 1
        print "[----------] %d tests from %s" % (len(somefiles), root[len(bpath)+1:])
        for name in somefiles:
            fullpath = os.path.join(root, name)
            name = fullpath[len(bpath)+1:]
            ok = run_test(name, fullpath)
            ntests += 1
            if ok:
                npassed += 1
            else:
                failed.append(name)
        print "[----------] %d tests from %s" % (len(somefiles), root[len(bpath)+1:])
    end = time.time()
    print "[==========] %d tests from %d directories ran. (%d ms total)" % (ntests, ndirs, int((end-start)*1000))
    print "[  PASSED  ] %d tests." % npassed
    if failed:
        print "[  FAILED  ] %d tests, listed below:" % len(failed)
        for name in failed:
            print "[  FAILED  ] %s" % name
        print " %d FAILED TESTS" % len(failed)
        return False
    return True

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 4:
        print >>sys.stderr, "Usage: rtest.py <path to regression tree>"
        print >>sys.stderr, "    <path to output directory>"
        print >>sys.stderr, "    <path to yasm executable>"
        sys.exit(2)
    outdir = sys.argv[2]
    yasmexe = sys.argv[3]
    all_ok = run_all(sys.argv[1])
    if all_ok:
        sys.exit(0)
    else:
        sys.exit(1)
