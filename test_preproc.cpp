#include <cstdlib>
#include <iostream>
#include <sstream>

#include "config.h"

#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/registry.h>
#include <yasmx/System/plugin.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Linemap.h>
#include <yasmx/Preprocessor.h>

using namespace yasm;

int
main()
{
    if (!LoadStandardPlugins())
        return EXIT_FAILURE;
    std::auto_ptr<PreprocessorModule> preproc_module =
        LoadModule<PreprocessorModule>("raw");
    std::string instr("test text");
    std::istringstream iss(instr);
    Linemap linemap;
    Errwarns errwarns;
    std::auto_ptr<Preprocessor> preproc = preproc_module->Create(errwarns);
    preproc->Initialize(iss, "<string>", linemap);

    std::string outstr;
    if (!preproc->getLine(outstr))
        return EXIT_FAILURE;

    std::cout << outstr << std::endl;

    if (outstr != instr)
        return EXIT_FAILURE;

    if (preproc->getLine(outstr))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
