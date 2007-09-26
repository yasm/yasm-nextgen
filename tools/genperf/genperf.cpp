/*
 * Generate Minimal Perfect Hash (genperf)
 *
 *  Copyright (C) 2006-2007  Peter Johnson
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
 */
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "perfect.h"

class Keyword {
public:
    Keyword(const std::string& n, const std::string& a, unsigned int l)
        : name(n), args(a), line(l)
    {}
    ~Keyword() {}
    std::string name;
    std::string args;
    unsigned int line;
};

static unsigned int cur_line = 1;
static int errors = 0;

static void
report_error(const std::string& err)
{
    std::cerr << cur_line << ": " << err << std::endl;
    errors++;
}

/* make the c output for the perfect hash tab array */
static void
make_c_tab(
    std::ostream& f,
    bstuff *tab,        /* table indexed by b */
    ub4 smax,           /* range of scramble[] */
    ub4 blen,           /* b in 0..blen-1, power of 2 */
    ub4 *scramble)      /* used in final hash */
{
    ub4   i;
    /* table for the mapping for the perfect hash */
    if (blen >= USE_SCRAMBLE) {
        /* A way to make the 1-byte values in tab bigger */
        if (smax > UB2MAXVAL+1) {
            f << "  static const unsigned long scramble[] = {\n";
            for (i=0; i<=UB1MAXVAL; i+=4)
                f << "    " << std::hex
                  << "0x" << scramble[i+0] << ','
                  << "0x" << scramble[i+1] << ','
                  << "0x" << scramble[i+2] << ','
                  << "0x" << scramble[i+3] << ",\n" << std::dec;
        } else {
            f << "  static const unsigned short scramble[] = {\n";
            for (i=0; i<=UB1MAXVAL; i+=8)
                f << "    " << std::hex
                  << "0x" << scramble[i+0] << ','
                  << "0x" << scramble[i+1] << ','
                  << "0x" << scramble[i+2] << ','
                  << "0x" << scramble[i+3] << ','
                  << "0x" << scramble[i+4] << ','
                  << "0x" << scramble[i+5] << ','
                  << "0x" << scramble[i+6] << ','
                  << "0x" << scramble[i+7] << ",\n" << std::dec;
        }
        f << "  };\n";
        f << '\n';
    }

    if (blen > 0) {
        /* small adjustments to _a_ to make values distinct */
        if (smax <= UB1MAXVAL+1 || blen >= USE_SCRAMBLE)
            f << "  static const unsigned char ";
        else
            f << "  static const unsigned short ";
        f << "tab[] = {\n";

        if (blen < 16) {
            for (i=0; i<blen; ++i)
                f << std::setw(3) << scramble[tab[i].val_b] << ',';
        } else if (blen <= 1024) {
            for (i=0; i<blen; i+=16)
                f << "    "
                  << scramble[tab[i+0].val_b] << ','
                  << scramble[tab[i+1].val_b] << ','
                  << scramble[tab[i+2].val_b] << ','
                  << scramble[tab[i+3].val_b] << ','
                  << scramble[tab[i+4].val_b] << ','
                  << scramble[tab[i+5].val_b] << ','
                  << scramble[tab[i+6].val_b] << ','
                  << scramble[tab[i+7].val_b] << ','
                  << scramble[tab[i+8].val_b] << ','
                  << scramble[tab[i+9].val_b] << ','
                  << scramble[tab[i+10].val_b] << ','
                  << scramble[tab[i+11].val_b] << ','
                  << scramble[tab[i+12].val_b] << ','
                  << scramble[tab[i+13].val_b] << ','
                  << scramble[tab[i+14].val_b] << ','
                  << scramble[tab[i+15].val_b] << ",\n";
        } else if (blen < USE_SCRAMBLE) {
            for (i=0; i<blen; i+=8)
                f << "    "
                  << scramble[tab[i+0].val_b] << ','
                  << scramble[tab[i+1].val_b] << ','
                  << scramble[tab[i+2].val_b] << ','
                  << scramble[tab[i+3].val_b] << ','
                  << scramble[tab[i+4].val_b] << ','
                  << scramble[tab[i+5].val_b] << ','
                  << scramble[tab[i+6].val_b] << ','
                  << scramble[tab[i+7].val_b] << ",\n";
        } else {
            for (i=0; i<blen; i+=16)
                f << "    "
                  << tab[i+0].val_b << ',' << tab[i+1].val_b << ','
                  << tab[i+2].val_b << ',' << tab[i+3].val_b << ','
                  << tab[i+4].val_b << ',' << tab[i+5].val_b << ','
                  << tab[i+6].val_b << ',' << tab[i+7].val_b << ','
                  << tab[i+8].val_b << ',' << tab[i+9].val_b << ','
                  << tab[i+10].val_b << ',' << tab[i+11].val_b << ','
                  << tab[i+12].val_b << ',' << tab[i+13].val_b << ','
                  << tab[i+14].val_b << ',' << tab[i+15].val_b << ",\n"; 
        }
        f << "  };\n";
        f << '\n';
    }
}

static void
perfect_gen(std::ostream& out,
            const std::string& language,
            const std::string& class_name,
            const std::string& lookup_function_name,
            const std::string& struct_name,
            std::vector<Keyword>& kws,
            const std::string& filename)
{
    ub4 nkeys;
    key *keys;
    hashform form;
    bstuff *tab;                /* table indexed by b */
    hstuff *tabh;               /* table indexed by hash value */
    ub4 smax;           /* scramble[] values in 0..smax-1, a power of 2 */
    ub4 alen;                   /* a in 0..alen-1, a power of 2 */
    ub4 blen;                   /* b in 0..blen-1, a power of 2 */
    ub4 salt;                   /* a parameter to the hash function */
    gencode final;              /* code for final hash */
    ub4 i;
    ub4 scramble[SCRAMBLE_LEN]; /* used in final hash function */
    char buf[10][80];           /* buffer for generated code */
    char *buf2[10];             /* also for generated code */

    /* perfect hash configuration */
    form.mode = hashform::NORMAL_HM;
    form.hashtype = hashform::STRING_HT;
    form.perfect = hashform::MINIMAL_HP;
    form.speed = hashform::SLOW_HS;

    /* set up code for final hash */
    final.line = buf2;
    final.used = 0;
    final.len  = 10;
    for (i=0; i<10; i++)
        final.line[i] = buf[i];

    /* build list of keys */
    nkeys = 0;
    keys = NULL;
    for (std::vector<Keyword>::iterator kw = kws.begin(), end = kws.end();
         kw != end; ++kw) {
        key *k = (key*)malloc(sizeof(key));

        k->name_k = (char*)malloc(kw->name.length()+1);
        std::strcpy(k->name_k, kw->name.c_str());
        k->len_k = (ub4)kw->name.length();
        k->next_k = keys;
        keys = k;
        nkeys++;
    }

    /* find the hash */
    findhash(&tab, &tabh, &alen, &blen, &salt, &final, 
             scramble, &smax, keys, nkeys, &form);

    /* The hash function beginning */
    if (language == "C++") {
        out << "class " << class_name << " {\n";
        out << "public:\n";
        out << "  static const struct " << struct_name << "* ";
        out << lookup_function_name << "(const char* key, size_t len);\n";
        out << "};\n\n";
        out << "const struct " << struct_name << "*\n";
        out << class_name << "::" << lookup_function_name
            << "(const char* key, size_t len)\n";
        out << "{\n";
    } else {
        out << "static const struct " << struct_name << " *\n";
        out << lookup_function_name << "(const char *key, size_t len)\n";
        out << "{\n";
    }

    /* output the dir table: this should loop up to smax for NORMAL_HP,
     * or up to pakd.nkeys for MINIMAL_HP.
     */
    out << "  static const struct " << struct_name
        << " pd[" << nkeys << "] = {\n";
    for (i=0; i<nkeys; i++) {
        if (tabh[i].key_h) {
            std::vector<Keyword>::iterator kw = kws.begin();
            for (std::vector<Keyword>::iterator end = kws.end();
                 kw != end; ++kw) {
                if (kw->name == tabh[i].key_h->name_k)
                    break;
            }
            if (kw == kws.end()) {
                std::ostringstream err;
                err << "internal error: could not find `"
                    << tabh[i].key_h->name_k << "'";
                report_error(err.str());
                break;
            }
            out << "#line " << kw->line << " \"" << filename << "\"\n";
            out << "    {\"" << kw->name << '"' << kw->args << '}';
        } else
            out << "    { NULL }";

        if (i < nkeys-1)
            out << ',';
        out << '\n';
    }
    out << "  };\n";

    /* output the hash tab[] array */
    make_c_tab(out, tab, smax, blen, scramble);

    /* The hash function body */
    out << "  const struct " << struct_name << " *ret;\n";
    for (i=0; i<final.used; ++i)
        out << final.line[i];
    out << "  if (rsl >= " << nkeys << ") return NULL;\n";
    out << "  ret = &pd[rsl];\n";
    out << "  if (strcmp(key, ret->name) != 0) return NULL;\n";
    out << "  return ret;\n";
    out << "}\n";
    out << "\n";

    free(tab);
    free(tabh);
}

int
main(int argc, char *argv[])
{
    std::ifstream in;
    std::ofstream out;
    char *ch;
    static char line[1024];
    std::string struct_name;
    std::string class_name = "Perfect_Hash";
    std::string lookup_function_name = "in_word_set";
    std::string language;
    std::string delimiters = ",\r\n";
    std::string name;
    std::string filename;
    bool need_struct = false;
    bool have_struct = false;
    bool go_keywords = false;
    bool ignore_case = false;
    bool compare_strncmp = false;
    bool readonly_tables = false;
    std::vector<std::string> usercode, usercode2;
    std::vector<Keyword> keywords;

    if (argc != 3) {
        std::cerr << "Usage: genperf <in> <out>" << std::endl;
        return EXIT_FAILURE;
    }

    in.open(argv[1], std::ifstream::in);
    if (in.fail()) {
        std::cerr << "Could not open `" << argv[1] << "' for reading"
                  << std::endl;
        return EXIT_FAILURE;
    }

    ch = argv[1];
    while (*ch) {
        if (*ch == '\\') {
            filename.push_back('/');
            ch++;
        } else
            filename.push_back(*ch++);
    }

    /* Parse declarations section */
    while (in.getline(line, 1024)) {
        /* Comments start with # as the first thing on a line */
        if (line[0] == '#') {
            cur_line++;
            continue;
        }

        /* Handle structure declaration */
        if (strncmp(line, "struct", 6) == 0) {
            int braces;

            if (!need_struct) {
                report_error("struct without %struct-type declaration");
                return EXIT_FAILURE;
            }
            if (have_struct) {
                report_error("more than one struct declaration");
                return EXIT_FAILURE;
            }
            have_struct = true;

            /* copy struct name */
            ch = &line[6];
            while (isspace(*ch))
                ch++;
            struct_name.clear();
            while (isalnum(*ch) || *ch == '_')
                struct_name.push_back(*ch++);

            std::ostringstream ostrline;
            ostrline << "#line " << cur_line << " \"" << filename << "\"\n";
            usercode.push_back(ostrline.str());

            braces = 0;
            do {
                /* count braces to determine when we're done */
                ch = line;
                while (*ch != '\0') {
                    if (*ch == '{')
                        braces++;
                    if (*ch == '}')
                        braces--;
                    ch++;
                }
                usercode.push_back(line);
                cur_line++;
                if (braces <= 0)
                    break;
            } while (in.getline(line, 1024));
            cur_line++;
            continue;
        }

        /* Ignore non-declaration lines */
        if (line[0] != '%') {
            cur_line++;
            continue;
        }

        /* %% terminates declarations section */
        if (line[1] == '%') {
            if (need_struct && !have_struct) {
                report_error("%struct-type declaration, but no struct found");
                return EXIT_FAILURE;
            }
            go_keywords = true;
            break;      /* move on to keywords section */
        }

        /* %{ begins a verbatim code section that ends with %} */
        if (line[1] == '{') {
            std::ostringstream ostrline;
            ostrline << "#line " << cur_line << " \"" << filename << "\"\n";
            usercode.push_back(ostrline.str());

            while (in.getline(line, 1024)) {
                cur_line++;
                if (line[0] == '%' && line[1] == '}')
                    break;
                usercode.push_back(line);
            }
            cur_line++;
            continue;
        }

        if (strncmp(&line[1], "ignore-case", 11) == 0) {
            ignore_case = true;
        } else if (strncmp(&line[1], "compare-strncmp", 15) == 0) {
            compare_strncmp = true;
        } else if (strncmp(&line[1], "readonly-tables", 15) == 0) {
            readonly_tables = true;
        } else if (strncmp(&line[1], "language=", 9) == 0) {
            ch = &line[10];
            language.clear();
            while (*ch != '\0')
                language.push_back(*ch++);
        } else if (strncmp(&line[1], "delimiters=", 11) == 0) {
            ch = &line[12];
            delimiters.clear();
            while (*ch != '\0')
                delimiters.push_back(*ch++);
        } else if (strncmp(&line[1], "enum", 4) == 0) {
            /* unused */
        } else if (strncmp(&line[1], "struct-type", 11) == 0) {
            need_struct = true;
        } else if (strncmp(&line[1], "define", 6) == 0) {
            /* Several different defines we need to handle */
            ch = &line[7];
            while (isspace(*ch))
                ch++;

            if (strncmp(ch, "hash-function-name", 18) == 0) {
                /* unused */
            } else if (strncmp(ch, "lookup-function-name", 20) == 0) {
                ch = &line[7+20+1];
                while (isspace(*ch))
                    ch++;
                lookup_function_name.clear();
                while (isalnum(*ch) || *ch == '_')
                    lookup_function_name.push_back(*ch++);
            } else if (strncmp(ch, "class-name", 10) == 0) {
                ch = &line[7+10+1];
                while (isspace(*ch))
                    ch++;
                class_name.clear();
                while (isalnum(*ch) || *ch == '_')
                    class_name.push_back(*ch++);
            } else {
                std::cerr << cur_line << ": unrecognized define `"
                          << line << "'" << std::endl;
            }
        } else {
            std::cerr << cur_line << ": unrecognized declaration `"
                      << line << "'" << std::endl;
        }

        cur_line++;
    }

    if (!go_keywords) {
        report_error("no keywords section found");
        return EXIT_FAILURE;
    }

    /* Parse keywords section */
    while (in.getline(line, 1024)) {
        char *d;

        /* Comments start with # as the first thing on a line */
        if (line[0] == '#') {
            cur_line++;
            continue;
        }

        /* Keywords section terminated with %% */
        if (line[0] == '%' && line[1] == '%')
            break;

        /* Look for name */
        ch = &line[0];
        name.clear();
        while (delimiters.find_first_of(*ch) == std::string::npos)
            name.push_back(*ch++);

        /* Strip EOL */
        d = std::strrchr(ch, '\n');
        if (d)
            *d = '\0';
        d = std::strrchr(ch, '\r');
        if (d)
            *d = '\0';
        keywords.push_back(Keyword(name, ch, cur_line));
        cur_line++;
    }

    if (errors > 0)
        return EXIT_FAILURE;

    /* Pull in any end code */
    if (!in.eof()) {
        std::ostringstream ostrline;
        ostrline << "#line " << cur_line << " \"" << filename << "\"\n";
        usercode2.push_back(ostrline.str());

        while (in.getline(line, 1024))
            usercode2.push_back(line);
    }

    /* output code */
    out.open(argv[2], std::ofstream::trunc);
    if (out.fail()) {
        std::cerr << "Could not open `" << argv[2] << "' for writing"
                  << std::endl;
        return EXIT_FAILURE;
    }

    out << "/* " << language << " code produced by genperf */\n";
    out << "/* Command-line: genperf " << argv[1] << ' ' << argv[2] << " */\n";

    for (std::vector<std::string>::iterator i = usercode.begin(),
         end = usercode.end(); i != end; ++i)
        out << *i << '\n';

    /* Get perfect hash */
    perfect_gen(out, language, class_name, lookup_function_name, struct_name,
                keywords, filename);

    for (std::vector<std::string>::iterator i = usercode2.begin(),
         end = usercode2.end(); i != end; ++i)
        out << *i << '\n';

    out.close();

    if (errors > 0) {
        remove(argv[2]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

