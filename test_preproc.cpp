#include <cstdlib>
#include <iostream>
#include <sstream>

#include "config.h"

#include <libyasmx/errwarn.h>
#include <libyasmx/errwarns.h>
#include <libyasmx/linemap.h>
#include <libyasmx/plugin.h>
#include <libyasmx/preproc.h>
#include <libyasmx/registry.h>

using namespace yasm;

int
main()
{
#ifdef BUILD_STATIC
    yasm_init_plugin();
#else
    if (!load_plugin("standard"))
        return EXIT_FAILURE;
#endif
    std::auto_ptr<Preprocessor> preproc = load_module<Preprocessor>("raw");
    std::string instr("test text");
    std::istringstream iss(instr);
    Linemap linemap;
    Errwarns errwarns;
    preproc->init(iss, "<string>", linemap, errwarns);

    std::string outstr;
    if (!preproc->get_line(outstr))
        return EXIT_FAILURE;

    std::cout << outstr << std::endl;

    if (outstr != instr)
        return EXIT_FAILURE;

    if (preproc->get_line(outstr))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
