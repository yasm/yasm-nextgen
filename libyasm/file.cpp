//
// File helper functions.
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
#include <util.h>

// Need either unistd.h or direct.h (on Windows) to prototype getcwd().
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#elif defined(HAVE_DIRECT_H)
#include <direct.h>
#endif

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <boost/lambda/lambda.hpp>

#include "errwarn.h"
#include "file.h"


namespace yasm {

bool
Scanner::fill_helper(unsigned char* &cursor,
                     boost::function<size_t (unsigned char*, size_t)> input_func)
{
    static const size_t BSIZE = 8192;       // Fill block size
    bool first = false;

    if (eof)
        return 0;

    size_t cnt = tok - bot;
    if (cnt > 0) {
        memmove(bot, tok, (size_t)(lim - tok));
        tok = bot;
        ptr -= cnt;
        cursor -= cnt;
        lim -= cnt;
    }
    if (!bot)
        first = true;
    if ((size_t)(top - lim) < BSIZE) {
        unsigned char *buf = new unsigned char[(size_t)(lim - bot) + BSIZE];
        memcpy(buf, tok, (size_t)(lim - tok));
        tok = buf;
        ptr = &buf[ptr - bot];
        cursor = &buf[cursor - bot];
        lim = &buf[lim - bot];
        top = &lim[BSIZE];
        delete bot;
        bot = buf;
    }
    if ((cnt = input_func(lim, BSIZE)) == 0) {
        eof = &lim[cnt];
        *eof++ = '\n';
    }
    lim += cnt;
    return first;
}

void
unescape_cstring(unsigned char* str, size_t &len)
{
    unsigned char* s = str;
    unsigned char* o = str;
    unsigned char t[4];

    while ((size_t)(s-str)<len) {
        if (*s == '\\' && (size_t)(&s[1]-str)<len) {
            s++;
            switch (*s) {
                case 'b': *o = '\b'; s++; break;
                case 'f': *o = '\f'; s++; break;
                case 'n': *o = '\n'; s++; break;
                case 'r': *o = '\r'; s++; break;
                case 't': *o = '\t'; s++; break;
                case 'x':
                    // hex escape; grab last two digits
                    s++;
                    while ((size_t)(&s[2]-str)<len && isxdigit(s[0])
                           && isxdigit(s[1]) && isxdigit(s[2]))
                        s++;
                    if ((size_t)(s-str)<len && isxdigit(*s)) {
                        t[0] = *s++;
                        t[1] = '\0';
                        t[2] = '\0';
                        if ((size_t)(s-str)<len && isxdigit(*s))
                            t[1] = *s++;
                        *o = (unsigned char)strtoul((char *)t, NULL, 16);
                    } else
                        *o = '\0';
                    break;
                default:
                    if (isdigit(*s)) {
                        int warn = 0;
                        // octal escape
                        if (*s > '7')
                            warn = 1;
                        *o = *s++ - '0';
                        if ((size_t)(s-str)<len && isdigit(*s)) {
                            if (*s > '7')
                                warn = 1;
                            *o <<= 3;
                            *o += *s++ - '0';
                            if ((size_t)(s-str)<len && isdigit(*s)) {
                                if (*s > '7')
                                    warn = 1;
                                *o <<= 3;
                                *o += *s++ - '0';
                            }
                        }
                        if (warn)
                            warn_set(WARN_GENERAL, N_("octal value out of range"));
                    } else
                        *o = *s++;
                    break;
            }
            o++;
        } else
            *o++ = *s++;
    }
    len = o-str;
}

std::string
splitpath_unix(const std::string& path, /*@out@*/ std::string& tail)
{
    using boost::lambda::_1;
    using boost::lambda::_2;

    std::string::size_type found = path.rfind('/');
    if (found == std::string::npos) {
        // No head
        tail = path;
        return "";
    }
    std::string head = path.substr(0, found+1);
    tail = path.substr(found+1);

    // Strip trailing ./ on path
    std::string::size_type len = head.length();
    while (len >= 2 && head[len-2] == '.' && head[len-1] == '/'
           && !(len >= 3 && head[len-3] == '.'))
        len -= 2;
    head.resize(len);

    // Strip trailing slashes on path (except leading)
    found = head.find_last_not_of('/');
    if (found != std::string::npos)
        head.erase(found+1);

    // Combine any double slashes
    std::string::iterator end =
        std::unique(head.begin(), head.end(), _1 == '/' && _2 == '/');
    head.erase(end, head.end());

    return head;
}

std::string
splitpath_win(const std::string& path, /*@out@*/ std::string& tail)
{
    using boost::lambda::_1;
    using boost::lambda::_2;

    std::string::size_type found = path.find_last_of("/\\");
    if (found == std::string::npos) {
        // look for drive letter
        if (path.length() >= 2 && std::isalpha(path[0]) && path[1] == ':') {
            tail = path.substr(2);
            return path.substr(0, 2);
        } else {
            tail = path;
            return "";
        }
    }
    std::string head = path.substr(0, found+1);
    tail = path.substr(found+1);

    // Replace all "/" with "\".
    std::replace(head.begin(), head.end(), '/', '\\');

    // Strip trailing .\ on path
    std::string::size_type len = head.length();
    while (len >= 2 && head[len-2] == '.' && head[len-1] == '\\'
           && !(len >= 3 && head[len-3] == '.'))
        len -= 2;
    head.resize(len);

    // Strip trailing slashes on path (except leading)
    found = head.find_last_not_of('\\');
    if (found != std::string::npos) {
        // don't strip slash immediately following drive letter
        if (found == 1 && std::isalpha(head[0]) && head[1] == ':')
            head.erase(found+2);
        else
            head.erase(found+1);
    }

    // Combine any double slashes
    std::string::iterator end =
        std::unique(head.begin(), head.end(), _1 == '\\' && _2 == '\\');
    head.erase(end, head.end());

    return head;
}

// FIXME: dumb way for now
std::string
abspath_unix(const std::string& path)
{
    char* curdir = getcwd(NULL, 0);

    std::string abspath = curdir;
    abspath += '/';
    abspath += path;

    free(curdir);

    return abspath;
}

// FIXME: dumb way for now
std::string
abspath_win(const std::string& path)
{
    char* curdir = getcwd(NULL, 0);

    std::string abspath = curdir;
    abspath += '\\';
    abspath += path;

    free(curdir);

    // Replace all "/" with "\".
    std::replace(abspath.begin(), abspath.end(), '/', '\\');

    return abspath;
}

std::string
combpath_unix(const std::string& from, const std::string& to)
{
    using boost::lambda::_1;
    using boost::lambda::_2;

    if (to[0] == '/') {
        // absolute "to"; combine any double slashes
        std::string out = to;
        std::string::iterator end =
            std::unique(out.begin(), out.end(), _1 == '/' && _2 == '/');
        out.erase(end, out.end());
        return out;
    }

    // Get path component; note this strips trailing slash
    std::string tail;
    std::string out = splitpath_unix(from, tail);

    // Add trailing slash back in
    if (!out.empty() && out[out.length()-1] != '/')
        out += '/';

    // Now scan from left to right through "to", stripping off "." and "..";
    // if we see "..", back up one directory in out unless last directory in
    // out is also "..".
    //
    // Note this does NOT back through ..'s in the "from" path; this is just
    // as well as that could skip symlinks (e.g. "foo/bar/.." might not be
    // the same as "foo").
    std::string::size_type i = 0, tolen = to.length();
    for (;;) {
        if ((tolen-i) >= 2 && to[i] == '.' && to[i+1] == '/') {
            i += 2;         // current directory
            while (to[i] == '/')
                i++;        // strip off any additional slashes
        } else if (out.empty())
            break;          // no more "from" path left, we're done
        else if ((tolen-i) >= 3 && to[i] == '.' && to[i+1] == '.' &&
                 to[i+2] == '/') {
            std::string::size_type outlen = out.length();

            if (outlen >= 3 && out[outlen-1] == '/' && out[outlen-2] == '.'
                && out[outlen-3] == '.') {
                // can't ".." against a "..", so we're done.
                break;
            }

            i += 3;     // throw away "../"
            while (to[i] == '/')
                i++;    // strip off any additional slashes

            // and back out last directory in "out" if not already at root
            if (outlen > 1) {
                std::string::size_type found = out.rfind('/', outlen-2);
                if (found != std::string::npos)
                    out.erase(found+1);
                else
                    out.clear();
            }
        } else
            break;
    }

    // Copy "to" to tail of output, and we're done
    out += to.substr(i);

    // Combine any double slashes before returning
    std::string::iterator end =
        std::unique(out.begin(), out.end(), _1 == '/' && _2 == '/');
    out.erase(end, out.end());

    return out;
}

std::string
combpath_win(const std::string& from, const std::string& to)
{
    using boost::lambda::_1;
    using boost::lambda::_2;

    if ((to.length() >= 2 && std::isalpha(to[0]) && to[1] == ':') ||
        to[0] == '/' || to[0] == '\\') {
        // absolute or drive letter "to"
        std::string out = to;

        // Replace all "/" with "\".
        std::replace(out.begin(), out.end(), '/', '\\');

        // combine any double slashes
        std::string::iterator end =
            std::unique(out.begin(), out.end(), _1 == '\\' && _2 == '\\');
        out.erase(end, out.end());

        return out;
    }

    // Get path component; note this strips trailing slash
    std::string tail;
    std::string out = splitpath_win(from, tail);

    // Add trailing slash back in, unless it's only a raw drive letter
    if (!out.empty()
        && out[out.length()-1] != '/' && out[out.length()-1] != '\\'
        && !(out.length() == 2 && std::isalpha(out[0]) && out[1] == ':'))
        out += '\\';

    // Now scan from left to right through "to", stripping off "." and "..";
    // if we see "..", back up one directory in out unless last directory in
    // out is also "..".
    //
    // Note this does NOT back through ..'s in the "from" path; this is just
    // as well as that could skip symlinks (e.g. "foo/bar/.." might not be
    // the same as "foo").
    std::string::size_type i = 0, tolen = to.length();
    for (;;) {
        if ((tolen-i) >= 2 && to[i] == '.'
            && (to[i+1] == '/' || to[i+1] == '\\')) {
            i += 2;         // current directory
            while (to[i] == '/' || to[i] == '\\')
                i++;        // strip off any additional slashes
        } else if (out.empty() ||
                   (out.length() == 2 && std::isalpha(out[0]) && out[1] == ':'))
            break;          // no more "from" path left, we're done
        else if ((tolen-i) >= 3 && to[i] == '.' && to[i+1] == '.'
                 && (to[i+2] == '/' || to[i+2] == '\\')) {
            std::string::size_type outlen = out.length();

            if (outlen >= 3 && (out[outlen-1] == '/' || out[outlen-1] == '\\')
                && out[outlen-2] == '.' && out[outlen-3] == '.') {
                // can't ".." against a "..", so we're done.
                break;
            }

            i += 3;     // throw away "../" (or "..\")
            while (to[i] == '/' || to[i] == '\\')
                i++;    // strip off any additional slashes

            // and back out last directory in "out" if not already at root
            if (outlen > 1 &&
                !(outlen == 3 && std::isalpha(out[0]) && out[1] == ':')) {
                std::string::size_type found =
                    out.find_last_of("/\\:", outlen-2);
                if (found != std::string::npos)
                    out.erase(found+1);
                else
                    out.clear();
            }
        } else
            break;
    }

    // Copy "to" to tail of output, and we're done
    out += to.substr(i);

    // Replace all "/" with "\".
    std::replace(out.begin(), out.end(), '/', '\\');

    // Combine any double slashes
    std::string::iterator end =
        std::unique(out.begin(), out.end(), _1 == '\\' && _2 == '\\');
    out.erase(end, out.end());

    return out;
}

std::string
Includes::open(std::ifstream& ifs, const std::string& iname,
               const std::string& from, std::ios_base::openmode mode) const
{
    // Close the stream if already open
    if (ifs.is_open())
        ifs.close();
    ifs.clear();

    // Try directly relative to from first, then each of the include paths.
    std::string combine = combpath(from, iname);
    ifs.open(combine.c_str(), mode);
    if (ifs)
        return combine;

    for (const_iterator i = begin(), e = end(); i != e; ++i) {
        combine = combpath(*i, iname);
        ifs.open(combine.c_str(), mode);
        if (ifs)
            return combine;
    }

    return "";
}

void
Includes::push_back(const std::string& path)
{
    // Add trailing slash if it is missing.
    std::string::size_type len = path.length();
    if (len > 0 && path[len-1] != '\\' && path[len-1] != '/')
        std::vector<std::string>::push_back(path+'/');
    else
        std::vector<std::string>::push_back(path);
}

} // namespace yasm
