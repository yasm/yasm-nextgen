#include <cstdlib>
#include <iostream>
#include <sstream>

#include "config.h"

#include <libyasmx/Support/registry.h>
#include <libyasmx/System/plugin.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Errwarns.h>
#include <libyasmx/Linemap.h>
#include <libyasmx/Preprocessor.h>

using namespace yasm;

int
main()
{
    if (!load_standard_plugins())
        return EXIT_FAILURE;
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
