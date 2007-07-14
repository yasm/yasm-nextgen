/**
 * \file libyasm/linemap.h
 * \brief YASM virtual line mapping interface.
 *
 * \license
 *  Copyright (C) 2002-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * \endlicense
 */
#ifndef YASM_LINEMAP_H
#define YASM_LINEMAP_H

namespace yasm {

class Bytecode;

class Linemap {
public:
    /** Create a new line mapping repository. */
    Linemap();

    /** Destructor. */
    ~Linemap();

    /** Get the current line position in a repository.
     * \return Current virtual line.
     */
    unsigned long get_current() { return m_current; }

    /** Get bytecode and source line information, if any, for a virtual line.
     * \param line          virtual line
     * \param bc            bytecode (output)
     * \param source        source code line pointer (output)
     * \return False if source line information available for line, true if
     *         not.
     * \note If source line information is not available, bc and source targets
     * are set to NULL.
     */
    bool get_source(unsigned long line,
                    /*@out@*/ /*@null@*/ Bytecode * &bc,
                    /*@out@*/ /*@null@*/ const char * &source) const;

    /** Add bytecode and source line information to the current virtual line.
     * \attention Deletes any existing bytecode and source line information for
     *            the current virtual line.
     * \param bc            bytecode (if any)
     * \param source        source code line
     * \note The source code line pointer is NOT kept, it is strdup'ed.
     */
    void add_source(/*@null@*/ Bytecode *bc, const char *source);

    /** Go to the next line (increments the current virtual line).
     * \return The current (new) virtual line.
     */
    unsigned long goto_next() { return ++m_current; }

    /** Set a new file/line physical association starting point at the current
     * virtual line.  line_inc indicates how much the "real" line is
     * incremented by for each virtual line increment (0 is perfectly legal).
     * \param filename      physical file name (if NULL, not changed)
     * \param file_line     physical line number
     * \param line_inc      line increment
     */
    void set(/*@null@*/ const char *filename,
             unsigned long file_line,
             unsigned long line_inc);

    /** Poke a single file/line association, restoring the original physical
     * association starting point.  Caution: increments the current virtual
     * line twice.
     * \param filename      physical file name (if NULL, not changed)
     * \param file_line     physical line number
     * \return The virtual line number of the poked association.
     */
    unsigned long poke(/*@null@*/ const char *filename,
                       unsigned long file_line);

    /** Look up the associated physical file and line for a virtual line.
     * \param line          virtual line
     * \param filename      physical file name (output)
     * \param file_line     physical line number (output)
     */
    void lookup(unsigned long line,
                /*@out@*/ const char * &filename,
                /*@out@*/ unsigned long &file_line) const;

    /** Traverses all filenames used in a linemap, calling a function on each
     * filename.
     * \param d             data pointer passed to func on each call
     * \param func          function
     * \return Stops early (and returns func's return value) if func returns a
     *         nonzero value; otherwise 0.
     */
    int traverse_filenames(/*@null@*/ void *d,
                           int (*func) (const char *filename, void *d)) const;

private:
    /** Current virtual line number. */
    unsigned long m_current;
};

} // namespace yasm

#endif
