//
// Generic Options Support Header File
//
// Copyright (c) 2001  Stanislav Karchebny <berk@madfire.net>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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
#include "yasm-options.h"

#include <util.h>

#include <cstring>
#include <iomanip>
#include <iostream>

#include <libyasm/compose.h>


// Options Parser
int
parse_cmdline(int argc,
              const char** argv,
              OptOption* options,
              size_t nopts,
              void (*print_error) (const std::string& msg))
{
    int errors = 0, warnings = 0;

fail:
    while (--argc)
    {
        argv++;

        if (argv[0][0] == '-')          // opt
        {
            bool got_it = false;
            if (argv[0][1] == '-')      // lopt
            {
                if (argv[0][2] == '\0')     // --, end of options
                {
                    // Handle rest of args as non-options
                    while (--argc)
                    {
                        argv++;
                        if (not_an_option_handler(argv[0]))
                            errors++;
                    }
                    return errors;
                }

                for (size_t i = 0; i < nopts; i++)
                {
                    if (options[i].lopt &&
                        std::strncmp(&argv[0][2], options[i].lopt,
                                     std::strlen(options[i].lopt)) == 0)
                    {
                        std::string cmd, param;

                        if (options[i].takes_param)
                        {
                            const char* contents = std::strchr(&argv[0][2], '=');
                            if (!contents)
                            {
                                print_error(String::compose(
                                    _("option `--%1' needs an argument!"),
                                    options[i].lopt));
                                errors++;
                                goto fail;
                            }
                            else
                            {
                                cmd.assign(&argv[0][2], contents-&argv[0][2]);
                                param.assign(contents+1);
                            }
                        }
                        else
                            cmd.assign(&argv[0][2]);

                        if (!options[i].
                            handler(&argv[0][2], param, options[i].extra))
                            got_it = true;
                        break;
                    }
                }
                if (!got_it && !other_option_handler(argv[0]))
                    got_it = true;
                if (!got_it)
                {
                    print_error(String::compose(
                        _("warning: unrecognized option `%1'"), argv[0]));
                    warnings++;
                }
            }
            else if (argv[0][1] == '\0')     // just -, is non-option
            {
                if (not_an_option_handler(argv[0]))
                    errors++;
            }
            else              // sopt
            {
                for (size_t i = 0; i < nopts; i++)
                {
                    if (argv[0][1] == options[i].sopt)
                    {
                        std::string cmd = &argv[0][1];
                        std::string param;

                        if (options[i].takes_param)
                        {
                            param = argv[1];
                            if (argv[0][2] != '\0')
                                param = &argv[0][2];
                            else if (param.empty() || param[0] == '-')
                            {
                                print_error(String::compose(
                                    _("option `-%1' needs an argument!"),
                                    options[i].sopt));
                                errors++;
                                goto fail;
                            }
                            else
                            {
                                argc--;
                                argv++;
                            }
                        }

                        if (!options[i].handler(cmd, param, options[i].extra))
                            got_it = true;
                        break;
                    }
                }
                if (!got_it && !other_option_handler(argv[0]))
                    got_it = true;
                if (!got_it)
                {
                    print_error(String::compose(
                        _("warning: unrecognized option `%1'"), argv[0]));
                    warnings++;
                }
            }
        }
        else      // not an option, then it should be a file or something
        {
            if (not_an_option_handler(argv[0]))
                errors++;
        }
    }

    return errors;
}

void
help_msg(const char* msg, const char* tail, OptOption* options, size_t nopts)
{
    std::cout << yasm_gettext(msg);

    for (size_t i = 0; i < nopts; i++)
    {
        std::string::size_type shortopt_len = 0;
        std::string::size_type longopt_len = 0;
        std::string optbuf, optopt;

        if (options[i].takes_param)
        {
            if (options[i].sopt)
            {
                optbuf += '-';
                optbuf += options[i].sopt;
                optbuf += " <";
                optbuf += options[i].param_desc ?
                    options[i].param_desc : _("param");
                optbuf += '>';
                shortopt_len = optbuf.length();
            }
            if (options[i].sopt && options[i].lopt)
                optbuf += ", ";
            if (options[i].lopt)
            {
                optopt = "--";
                optopt += options[i].lopt;
                optopt += "=<";
                optopt += options[i].param_desc ?
                    options[i].param_desc : _("param");
                optopt += '>';
                optbuf += optopt;
                longopt_len = optbuf.length();
            }
        }
        else
        {
            if (options[i].sopt)
            {
                optbuf += '-';
                optbuf += options[i].sopt;
                shortopt_len = optbuf.length();
            }
            if (options[i].sopt && options[i].lopt)
                optbuf += ", ";
            if (options[i].lopt)
            {
                optopt = "--";
                optopt += options[i].lopt;
                optbuf += optopt;
                longopt_len = optbuf.length();
            }
        }

        // split [-s <desc>], [--long <desc>] if it destroys columns
        if (shortopt_len && longopt_len && longopt_len > 22)
        {
            optbuf.resize(shortopt_len);
            std::cout << "    " << std::left << std::setw(22) << optopt << "  "
                      << yasm_gettext(options[i].description) << std::endl;
            std::cout << "     " << optbuf << std::endl;
        }
        else
            std::cout << "    " << std::left << std::setw(22) << optbuf << "  "
                      << yasm_gettext(options[i].description) << std::endl;
    }

    std::cout << yasm_gettext(tail);
}
