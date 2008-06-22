#ifndef YASM_EXPORT_H
#define YASM_EXPORT_H

#cmakedefine YASM_HAVE_GCC_VISIBILITY 1

#ifndef BUILD_STATIC
#cmakedefine BUILD_STATIC 1
#endif

#ifndef YASM_EXPORT
# ifdef BUILD_STATIC
#  define YASM_EXPORT
#  define YASM_IMPORT
# elif defined(WIN32) || defined(_MSC_VER)
#  define YASM_EXPORT __declspec(dllexport)
#  define YASM_IMPORT __declspec(dllimport)
# elif YASM_HAVE_GCC_VISIBILITY
#  define YASM_EXPORT __attribute__((visibility("default")))
#  define YASM_IMPORT __attribute__((visibility("default")))
# endif


# ifndef YASM_EXPORT
#  define YASM_EXPORT
#  define YASM_IMPORT
# endif
#endif

#ifndef YASM_LIB_EXPORT
# ifdef MAKE_LIBYASMX_LIB
#  define YASM_LIB_EXPORT YASM_EXPORT
# else
#  define YASM_LIB_EXPORT YASM_IMPORT
# endif
#endif

#endif
