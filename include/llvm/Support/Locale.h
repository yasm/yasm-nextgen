#ifndef LLVM_SUPPORT_LOCALE
#define LLVM_SUPPORT_LOCALE

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"

namespace llvm {
namespace sys {
namespace locale {

YASM_LIB_EXPORT
int columnWidth(StringRef s);
YASM_LIB_EXPORT
bool isPrint(int c);

}
}
}

#endif // LLVM_SUPPORT_LOCALE
