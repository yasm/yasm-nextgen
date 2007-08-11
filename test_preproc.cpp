#include <iostream>
#include <sstream>

#include <libyasm/errwarn.h>
#include <libyasm/factory.h>
#include <libyasm/linemap.h>
#include <libyasm/preproc.h>

using namespace yasm;

int
main()
{
    std::auto_ptr<Preprocessor> preproc =
        ddj::genericFactory<Preprocessor>::instance().create("raw");

    std::string instr("test text");
    std::istringstream iss(instr);
    Linemap linemap;
    Errwarns errwarns;
    preproc->init(iss, "<string>", linemap, errwarns);

    std::istream& ppstream = preproc->get_stream();

    if (ppstream.eof())
        return EXIT_FAILURE;

    std::string outstr;
    getline(ppstream, outstr);

    std::cout << outstr << std::endl;

    if (outstr != instr)
        return EXIT_FAILURE;

    if (!ppstream.eof())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
