// arch_x86 dbgfmt_dwarf2 dbgfmt_null objfmt_bin objfmt_elf parser_gas 
// rev 5
#include <yasmx/System/plugin.h>

void yasm_arch_x86_DoRegister();
void yasm_dbgfmt_dwarf2_DoRegister();
void yasm_dbgfmt_null_DoRegister();
void yasm_objfmt_bin_DoRegister();
void yasm_objfmt_elf_DoRegister();
void yasm_parser_gas_DoRegister();

bool
LoadStandardPlugins()
{
    yasm_arch_x86_DoRegister();
    yasm_dbgfmt_dwarf2_DoRegister();
    yasm_dbgfmt_null_DoRegister();
    yasm_objfmt_bin_DoRegister();
    yasm_objfmt_elf_DoRegister();
    yasm_parser_gas_DoRegister();
    return true;
}
