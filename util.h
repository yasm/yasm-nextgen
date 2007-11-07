#ifndef YASM_UTIL_H
#define YASM_UTIL_H
//
// YASM utility functions.
//
// Includes standard headers and defines prototypes for replacement functions
// if needed.
//
//  Copyright (C) 2001-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>

#ifdef lint
# define _(String)      String
#else
# ifdef HAVE_LOCALE_H
#  include <locale.h>
# endif

# ifdef ENABLE_NLS
#  include <libintl.h>
#  define yasm_gettext(Msgid)                       gettext(Msgid)
#  define yasm_textdomain(Domainname)               textdomain(Domainname)
#  define yasm_bindtextdomain(Domainname, Dirname)  \
    bindtextdomain(Domainname, Dirname)
#  define _(String)     gettext(String)
# else
#  define yasm_gettext(Msgid)                       (Msgid)
#  define yasm_textdomain(Domainname)               while (0) /* nothing */
#  define yasm_bindtextdomain(Domainname, Dirname)  while (0) /* nothing */
#  define _(String)     (String)
# endif
#endif

#ifdef gettext_noop
# define N_(String)     gettext_noop(String)
#else
# define N_(String)     (String)
#endif

#ifndef NELEMS
/** Get the number of elements in an array.
 * \internal
 * \param array     array
 * \return Number of elements.
 */
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

#endif
