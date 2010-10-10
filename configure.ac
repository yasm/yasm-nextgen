# Process this file with autoconf to produce a configure script.

#
# autoconf setup
#
AC_PREREQ(2.53)
AC_INIT([ygas], [HEAD], [bug-yasm@tortall.net])
AC_CONFIG_SRCDIR(frontends/ygas.cpp)
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET
AC_LANG([C++])

AM_INIT_AUTOMAKE([1.9.6 foreign])
AM_MAINTAINER_MODE

AC_DEFINE(PACKAGE_INTVER, ["1.1.99"],
	  [Define to internal version of this package.])
AC_DEFINE(PACKAGE_BUILD, ["HEAD"], [Define to build version of this package.])

AC_DEFINE(LLVM_ON_UNIX, [1], [Define if building on unix.])

AC_MSG_CHECKING([YGAS_OBJFMT_BITS])
if echo $target_cpu | grep -Eq '^x86_64' ; then
  YGAS_OBJFMT_BITS=64
elif echo $target_cpu | grep -Eq '^i[0-9]86' ; then
  YGAS_OBJFMT_BITS=32
else
  AC_MSG_WARN([Could not determine YGAS_OBJFMT_BITS from target "$target_cpu"])
  YGAS_OBJFMT_BITS=32
fi
AC_MSG_RESULT([${YGAS_OBJFMT_BITS}])
AC_DEFINE([YGAS_OBJFMT_BITS], ["${YGAS_OBJFMT_BITS}"],
	  [Define to default ygas bits])

AC_MSG_CHECKING([YGAS_OBJFMT_BASE])
YGAS_OBJFMT_BASE=elf
AC_MSG_RESULT([${YGAS_OBJFMT_BASE}])
AC_DEFINE([YGAS_OBJFMT_BASE], ["${YGAS_OBJFMT_BASE}"],
	  [Define to default ygas object format])

#
# Checks for programs.
#
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Check for compiler output filename suffixes.
AC_OBJEXT
AC_EXEEXT

#
# Checks for header files.
#
AC_HEADER_ASSERT
AC_HEADER_DIRENT
AC_HEADER_MMAP_ANONYMOUS
AC_HEADER_STAT
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([errno.h strings.h libgen.h unistd.h direct.h sys/stat.h])
AC_CHECK_HEADERS([fcntl.h malloc.h malloc/malloc.h memory.h utime.h])
AC_CHECK_HEADERS([sys/ioctl.h termios.h stdint.h sys/types.h inttypes.h])
AC_CHECK_HEADERS([unistd.h sys/time.h functional tr1/functional sys/mman.h])
AC_CHECK_HEADERS([sys/resource.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([error_t], [], [], [[#include <errno.h>]])
AC_CHECK_TYPES([uint64_t, u_int64_t], [], [],
	       [[#include <sys/types.h>], [#include <inttypes.h>],
	        [#include <stdint.h>]])

# functional
AC_DEFUN([CHECK_STD_FUNCTIONAL],
[AC_CACHE_CHECK([for $2 in functional], $3,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <functional>
      #include <vector>
      #include <algorithm>
      struct X { void f() {} };
      void f2(int y) {}]],
    [[$1]])],
    [$3=yes],[$3=no]))])
AC_DEFUN([CHECK_TR1_FUNCTIONAL],
[AC_CACHE_CHECK([for $2 in tr1/functional], $3,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <tr1/functional>
      #include <vector>
      #include <algorithm>
      struct X { void f() {} };
      void f2(int y) {}]],
    [[$1]])],
    [$3=yes],[$3=no]))])

# checks for function, ref, bind, and mem_fn
if test $ac_cv_header_tr1_functional = yes ; then
  CHECK_TR1_FUNCTIONAL([[std::function<void (int)> x = f2;]],
                       [[std::function]], ac_cv_std_function1)
  CHECK_TR1_FUNCTIONAL([[std::tr1::function<void (int)> x = f2;]],
                       [[std::tr1::function]], ac_cv_tr1_function1)
  CHECK_TR1_FUNCTIONAL([[X x1; X& x2 = std::ref(x1);]],
                       [[std::ref]], ac_cv_std_ref1)
  CHECK_TR1_FUNCTIONAL([[X x1; X& x2 = std::tr1::ref(x1);]],
                       [[std::tr1::ref]], ac_cv_tr1_ref1)
  CHECK_TR1_FUNCTIONAL([[std::bind(f2, 1)();]],
                       [[std::bind]], ac_cv_std_bind1)
  CHECK_TR1_FUNCTIONAL([[std::tr1::bind(f2, 1)();]],
                       [[std::tr1::bind]], ac_cv_tr1_bind1)
  CHECK_TR1_FUNCTIONAL([[std::vector<X> v; std::for_each(v.begin(), v.end(), std::mem_fn(&X::f));]],
                       [[std::mem_fn]], ac_cv_std_mem_fn1)
  CHECK_TR1_FUNCTIONAL([[std::vector<X> v; std::for_each(v.begin(), v.end(), std::tr1::mem_fn(&X::f));]],
                       [[std::tr1::mem_fn]], ac_cv_tr1_mem_fn1)
fi

if test $ac_cv_header_functional = yes ; then
  CHECK_STD_FUNCTIONAL([[std::function<void (int)> x = f2;]],
                       [[std::function]], ac_cv_std_function2)
  CHECK_STD_FUNCTIONAL([[std::tr1::function<void (int)> x = f2;]],
                       [[std::tr1::function]], ac_cv_tr1_function2)
  CHECK_STD_FUNCTIONAL([[X x1; X& x2 = std::ref(x1);]],
                       [[std::ref]], ac_cv_std_ref2)
  CHECK_STD_FUNCTIONAL([[X x1; X& x2 = std::tr1::ref(x1);]],
                       [[std::tr1::ref]], ac_cv_tr1_ref2)
  CHECK_STD_FUNCTIONAL([[std::bind(f2, 1)();]],
                       [[std::bind]], ac_cv_std_bind2)
  CHECK_STD_FUNCTIONAL([[std::tr1::bind(f2, 1)();]],
                       [[std::tr1::bind]], ac_cv_tr1_bind2)
  CHECK_STD_FUNCTIONAL([[std::vector<X> v; std::for_each(v.begin(), v.end(), std::mem_fn(&X::f));]],
                       [[std::mem_fn]], ac_cv_std_mem_fn2)
  CHECK_STD_FUNCTIONAL([[std::vector<X> v; std::for_each(v.begin(), v.end(), std::tr1::mem_fn(&X::f));]],
                       [[std::tr1::mem_fn]], ac_cv_tr1_mem_fn2)
fi

AC_MSG_CHECKING([Using functional])
if test $ac_cv_std_function2 = yes -a $ac_cv_std_ref2 = yes -a \
        $ac_cv_std_bind2 = yes -a $ac_cv_std_mem_fn2 = yes ; then
  tr1_namespace=std
  tr1_header=functional
elif test $ac_cv_tr1_function2 = yes -a $ac_cv_tr1_ref2 = yes -a \
          $ac_cv_tr1_bind2 = yes -a $ac_cv_tr1_mem_fn2 = yes ; then
  tr1_namespace=std::tr1
  tr1_header=functional
elif test $ac_cv_std_function1 = yes -a $ac_cv_std_ref1 = yes -a \
          $ac_cv_std_bind1 = yes -a $ac_cv_std_mem_fn1 = yes ; then
  tr1_namespace=std
  tr1_header=tr1/functional
elif test $ac_cv_tr1_function1 = yes -a $ac_cv_tr1_ref1 = yes -a \
          $ac_cv_tr1_bind1 = yes -a $ac_cv_tr1_mem_fn1 = yes ; then
  tr1_namespace=std::tr1
  tr1_header=tr1/functional
else
  tr1_namespace=boost
  tr1_header=boost
fi
AC_MSG_RESULT([$tr1_namespace:: from $tr1_header])

rm -f functional_includes
rm -f functional_aliases
if test tr1_header = boost ; then
  cat <<EOF > functional_includes
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
EOF
  cat <<EOF > functional_aliases
namespace FUNCTION = boost;
namespace REF = boost;
namespace BIND = boost;
namespace MEMFN = boost;
EOF
else
  cat <<EOF > functional_includes
#include <$tr1_header>
EOF
  cat <<EOF > functional_aliases
namespace FUNCTION = $tr1_namespace;
namespace REF = $tr1_namespace;
namespace BIND = $tr1_namespace;
using namespace $tr1_namespace::placeholders;
namespace MEMFN = $tr1_namespace;
EOF
fi
FUNCTIONAL_INCLUDES=functional_includes
FUNCTIONAL_ALIASES=functional_aliases
AC_SUBST_FILE(FUNCTIONAL_INCLUDES)
AC_SUBST_FILE(FUNCTIONAL_ALIASES)

#
# Checks for library functions.
#
AC_CHECK_FUNCS([getcwd getpagesize getrlimit getrusage gettimeofday index])
AC_CHECK_FUNCS([isatty malloc_zone_statistics memcpy memmove])
AC_CHECK_FUNCS([mkdtemp mkstemp mktemp opendir readdir realpath rindex sbrk])
AC_CHECK_FUNCS([setenv setjmp getrlimit setrlimit shl_load sysconf mallinfo])
AC_CHECK_FUNCS([strerror])
AC_FUNC_MMAP
AC_FUNC_MMAP_FILE
AC_NEED_DEV_ZERO_FOR_MMAP

# Cross-build support
AC_ARG_VAR(CC_FOR_BUILD,[build system C compiler])
AC_ARG_VAR(CXX_FOR_BUILD,[build system C++ compiler])
AC_ARG_VAR(CCLD_FOR_BUILD,[build system C linker frontend])
AC_ARG_VAR(CXXLD_FOR_BUILD,[build system C++ linker frontend])
if test "${build}" != "${host}" ; then
  CC_FOR_BUILD=${CC_FOR_BUILD-cc}
  CXX_FOR_BUILD=${CXX_FOR_BUILD-c++}
  CCLD_FOR_BUILD=${CCLD_FOR_BUILD-cc}
  CXXLD_FOR_BUILD=${CXXLD_FOR_BUILD-c++}
else
  CC_FOR_BUILD="\$(CC)"
  CXX_FOR_BUILD="\$(CXX)"
  CCLD_FOR_BUILD="\$(CC)"
  CXXLD_FOR_BUILD="\$(CXX)"
fi
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CXX_FOR_BUILD)
AC_SUBST(CCLD_FOR_BUILD)
AC_SUBST(CXXLD_FOR_BUILD)

# Detect if we have Python
AC_MSG_NOTICE([Checking for Python])
have_python=no
AM_PATH_PYTHON(2.4,[],[AC_MSG_WARN([Python not found])])

if test -z "$PYTHON" || test "$PYTHON" = : ; then
    have_python=no
else
    have_python=yes
fi

if test x$have_python = xno ; then
    AC_MSG_ERROR([Could not find a suitable version of Python])
fi

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([include/yasmx/Config/functional.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
