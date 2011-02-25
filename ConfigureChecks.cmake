INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckTypeSize)
INCLUDE(TestCXXAcceptsFlag)
INCLUDE(CMakeBackwardCompatibilityCXX)
INCLUDE(MacroEnsureVersion)

# Helper macros and functions
macro(add_cxx_include result files)
  set(${result} "")
  foreach (file_name ${files})
     set(${result} "${${result}}#include<${file_name}>\n")
  endforeach()
endmacro(add_cxx_include files result)

function(check_type_exists type files variable)
  add_cxx_include(includes "${files}")
  CHECK_CXX_SOURCE_COMPILES("
    ${includes} ${type} typeVar;
    int main() {
        return 0;
    }
    " ${variable})
endfunction()

#FIND_PROGRAM(XMLTO NAMES xmlto)
#IF (XMLTO)
#    SET(BUILD_MAN ON)
#ENDIF (XMLTO)

# Platform-specific include files (POSIX, Win32)
INCLUDE(CheckIncludeFile)
check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(argz.h HAVE_ARGZ_H)
check_include_file(assert.h HAVE_ASSERT_H)
check_include_file(direct.h HAVE_DIRECT_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(dl.h HAVE_DL_H)
check_include_file(dld.h HAVE_DLD_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(errno.h HAVE_ERRNO_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(libgen.h HAVE_LIBGEN_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(link.h HAVE_LINK_H)
check_include_file(locale.h HAVE_LOCALE_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(malloc/malloc.h HAVE_MALLOC_MALLOC_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(ndir.h HAVE_NDIR_H)
if( NOT LLVM_ON_WIN32 )
  check_include_file(pthread.h HAVE_PTHREAD_H)
endif()
check_include_file(setjmp.h HAVE_SETJMP_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdio.h HAVE_STDIO_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/dir.h HAVE_SYS_DIR_H)
check_include_file(sys/dl.h HAVE_SYS_DL_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(termios.h HAVE_TERMIOS_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(utime.h HAVE_UTIME_H)
check_include_file(valgrind/valgrind.h HAVE_VALGRIND_VALGRIND_H)
check_include_file(windows.h HAVE_WINDOWS_H)

# library checks
INCLUDE(CheckLibraryExists)
check_library_exists(pthread pthread_create "" HAVE_LIBPTHREAD)
check_library_exists(dl dlopen "" HAVE_LIBDL)

IF (HAVE_LIBDL)
    SET(LIBDL "dl")
ELSE (HAVE_LIBDL)
    SET(LIBDL "")
ENDIF (HAVE_LIBDL)

# function checks
INCLUDE(CheckSymbolExists)
INCLUDE(CheckFunctionExists)

check_symbol_exists(__GLIBC__ stdio.h LLVM_USING_GLIBC)
if( LLVM_USING_GLIBC )
  add_definitions( -D_GNU_SOURCE )
endif( LLVM_USING_GLIBC )

check_symbol_exists(abort stdlib.h HAVE_ABORT)
check_function_exists(getcwd HAVE_GETCWD)
check_symbol_exists(alloca alloca.h HAVE_ALLOCA)
check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
check_symbol_exists(getrusage sys/resource.h HAVE_GETRUSAGE)
check_symbol_exists(getrlimit sys/resource.h HAVE_GETRLIMIT)
check_symbol_exists(setrlimit sys/resource.h HAVE_SETRLIMIT)
check_function_exists(isatty HAVE_ISATTY)
check_symbol_exists(isinf cmath HAVE_ISINF_IN_CMATH)
check_symbol_exists(isinf math.h HAVE_ISINF_IN_MATH_H)
check_symbol_exists(isnan cmath HAVE_ISNAN_IN_CMATH)
check_symbol_exists(isnan math.h HAVE_ISNAN_IN_MATH_H)
check_symbol_exists(ceilf math.h HAVE_CEILF)
check_symbol_exists(floorf math.h HAVE_FLOORF)
check_symbol_exists(mallinfo malloc.h HAVE_MALLINFO)
check_symbol_exists(malloc_zone_statistics malloc/malloc.h
                    HAVE_MALLOC_ZONE_STATISTICS)
check_symbol_exists(mkdtemp "stdlib.h;unistd.h" HAVE_MKDTEMP)
check_symbol_exists(mkstemp "stdlib.h;unistd.h" HAVE_MKSTEMP)
check_symbol_exists(mktemp "stdlib.h;unistd.h" HAVE_MKTEMP)
if( NOT LLVM_ON_WIN32 )
  check_symbol_exists(pthread_mutex_lock pthread.h HAVE_PTHREAD_MUTEX_LOCK)
endif()
check_symbol_exists(sbrk unistd.h HAVE_SBRK)
check_symbol_exists(strdup string.h HAVE_STRDUP)
check_symbol_exists(strerror string.h HAVE_STRERROR)
check_symbol_exists(strerror_r string.h HAVE_STRERROR_R)
check_symbol_exists(strtoll stdlib.h HAVE_STRTOLL)

if (CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_REQUIRED_FLAGS "-std=c99")
  SET(CMAKE_REQUIRED_LIBRARIES m)
endif (CMAKE_COMPILER_IS_GNUCXX)
check_function_exists(round HAVE_ROUND)

# Type checks
check_type_size("long long" LONG_LONG)
check_type_size("unsigned long long" UNSIGNED_LONG_LONG)
IF (HAVE_LONG_LONG AND HAVE_UNSIGNED_LONG_LONG)
    SET(YASM_HAVE_LONG_LONG 1)
ENDIF (HAVE_LONG_LONG AND HAVE_UNSIGNED_LONG_LONG)

set(headers "")
if (HAVE_SYS_TYPES_H)
  set(headers ${headers} "sys/types.h")
endif()

if (HAVE_INTTYPES_H)
  set(headers ${headers} "inttypes.h")
endif()

if (HAVE_STDINT_H)
  set(headers ${headers} "stdint.h")
endif()

check_type_exists(uint64_t "${headers}" HAVE_UINT64_T)
check_type_exists(u_int64_t "${headers}" HAVE_U_INT64_T)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fPIC" SUPPORTS_FPIC_FLAG)

include(GetTargetTriple)
get_target_triple(YASM_HOSTTRIPLE)
message(STATUS "YASM_HOSTTRIPLE: ${YASM_HOSTTRIPLE}")

# Set YGAS_OBJFMT_BASE and YGAS_OBJFMT_BITS based on YASM_HOSTTRIPLE
if(YASM_HOSTTRIPLE MATCHES "^x86_64")
  set(YGAS_OBJFMT_BITS "64")
elseif(YASM_HOSTTRIPLE MATCHES "^i[0-9]86")
  set(YGAS_OBJFMT_BITS "32")
else(YASM_HOSTTRIPLE MATCHES "^x86_64")
  message(WARNING "Could not determine YGAS_OBJFMT_BITS from YASM_HOSTTRIPLE")
  set(YGAS_OBJFMT_BITS "32")
endif(YASM_HOSTTRIPLE MATCHES "^x86_64")
message(STATUS "YGAS_OBJFMT_BITS: ${YGAS_OBJFMT_BITS}")

if(YASM_HOSTTRIPLE MATCHES "win32" OR YASM_HOSTTRIPLE MATCHES "mingw32")
  set(YGAS_OBJFMT_BASE "win")
else(YASM_HOSTTRIPLE MATCHES "win32" OR YASM_HOSTTRIPLE MATCHES "mingw32")
  set(YGAS_OBJFMT_BASE "elf")
endif(YASM_HOSTTRIPLE MATCHES "win32" OR YASM_HOSTTRIPLE MATCHES "mingw32")
message(STATUS "YGAS_OBJFMT_BASE: ${YGAS_OBJFMT_BASE}")

set(LIBPSAPI "")
set(LIBIMAGEHLP "")
if( MINGW )
  set(HAVE_LIBIMAGEHLP 1)
  set(HAVE_LIBPSAPI 1)
  # TODO: Check existence of libraries.
  #   include(CheckLibraryExists)
  #   CHECK_LIBRARY_EXISTS(imagehlp ??? . HAVE_LIBIMAGEHLP)
  set (LIBPSAPI "psapi")
  set (LIBIMAGEHLP "imagehlp")
endif( MINGW )

if( MSVC )
  set(error_t int)
  set(mode_t "unsigned short")
  set(LTDL_SHLIBPATH_VAR "PATH")
  set(LTDL_SYSSEARCHPATH "")
  set(LTDL_DLOPEN_DEPLIBS 1)
  set(SHLIBEXT ".lib")
  set(LTDL_OBJDIR "_libs")
  set(HAVE_STRTOLL 1)
  set(strtoll "_strtoi64")
  set(strtoull "_strtoui64")
  set(stricmp "_stricmp")
  set(strdup "_strdup")
else( MSVC )
  set(LTDL_SHLIBPATH_VAR "LD_LIBRARY_PATH")
  set(LTDL_SYSSEARCHPATH "") # TODO
  set(LTDL_DLOPEN_DEPLIBS 0)  # TODO
endif( MSVC )

# FIXME: Signal handler return type, currently hardcoded to 'void'
set(RETSIGTYPE void)

#if( LLVM_ENABLE_THREADS )
#  if( HAVE_PTHREAD_H OR WIN32 )
#    set(ENABLE_THREADS 1)
#  endif( HAVE_PTHREAD_H OR WIN32 )
#endif( LLVM_ENABLE_THREADS )
#
#if( ENABLE_THREADS )
#  message(STATUS "Threads enabled.")
#else( ENABLE_THREADS )
#  message(STATUS "Threads disabled.")
#endif( ENABLE_THREADS )
set(LLVM_MULTITHREADED 0)

if(WIN32)
  if(CYGWIN)
    set(LLVM_ON_WIN32 0)
    set(LLVM_ON_UNIX 1)
  else(CYGWIN)
    set(LLVM_ON_WIN32 1)
    set(LLVM_ON_UNIX 0)
  endif(CYGWIN)
  set(LTDL_SHLIB_EXT ".dll")
  set(EXEEXT ".exe")
  # Maximum path length is 160 for non-unicode paths
  set(MAXPATHLEN 160)
else(WIN32)
  if(UNIX)
    set(LLVM_ON_WIN32 0)
    set(LLVM_ON_UNIX 1)
    set(LTDL_SHLIB_EXT ".so")
    set(EXEEXT "")
    # FIXME: Maximum path length is currently set to 'safe' fixed value
    set(MAXPATHLEN 2024)
  else(UNIX)
    MESSAGE(SEND_ERROR "Unable to determine platform")
  endif(UNIX)
endif(WIN32)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
INCLUDE(FindPythonInterp)
#INCLUDE(FindPythonLibs)

#IF (PYTHONINTERP_FOUND)
#    EXEC_PROGRAM("${PYTHON_EXECUTABLE}"
#                 ARGS "${yasm_SOURCE_DIR}/CMake/have_pyrex.py"
#                 RETURN_VALUE HAVE_PYREX)
#ENDIF (PYTHONINTERP_FOUND)

######################################################
#  and now the platform specific stuff
######################################################

# setup default RPATH/install_name handling, may be overridden by YASM_HANDLE_RPATH_FOR_[LIBRARY|EXECUTABLE]
# default is to build with RPATH for the install dir, so it doesn't need to relink
if (UNIX)
   if (NOT APPLE)
     set( _YASM_DEFAULT_USE_FULL_RPATH ON )
   else (NOT APPLE)
     set( _YASM_DEFAULT_USE_FULL_RPATH OFF )
   endif (NOT APPLE)

   option(YASM_USE_ALWAYS_FULL_RPATH "If set to TRUE, also libs and plugins will be linked with the full RPATH, which will usually make them work better, but make install will take longer." ${_YASM_DEFAULT_USE_FULL_RPATH} )

   set( _YASM_PLATFORM_INCLUDE_DIRS)

   # the rest is RPATH handling
   # here the defaults are set
   # which are partly overwritten in yasm_handle_rpath_for_library()
   # and yasm_handle_rpath_for_executable(), both located in YasmMacros.cmake, Alex
   if (APPLE)
      set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
   else (APPLE)
      # add our LIB_INSTALL_DIR to the RPATH and use the RPATH figured out by cmake when compiling
      set(CMAKE_INSTALL_RPATH ${LIB_INSTALL_DIR} )
      set(CMAKE_SKIP_BUILD_RPATH TRUE)
      set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
      set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
   endif (APPLE)
endif (UNIX)

if (APPLE)

  set (CMAKE_SHARED_LINKER_FLAGS "-single_module -multiply_defined suppress ${CMAKE_SHARED_LINKER_FLAGS}")
  set (CMAKE_MODULE_LINKER_FLAGS "-multiply_defined suppress ${CMAKE_MODULE_LINKER_FLAGS}")

  set (CMAKE_C_FLAGS     "${CMAKE_C_FLAGS} -fno-common")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
endif (APPLE)


if (CMAKE_SYSTEM_NAME MATCHES Linux)
   if (CMAKE_COMPILER_IS_GNUCXX)
      set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
      set ( CMAKE_MODULE_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_MODULE_LINKER_FLAGS}")

      set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-new-dtags ${CMAKE_SHARED_LINKER_FLAGS}")
      set ( CMAKE_MODULE_LINKER_FLAGS "-Wl,--enable-new-dtags ${CMAKE_MODULE_LINKER_FLAGS}")
      set ( CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags ${CMAKE_EXE_LINKER_FLAGS}")

   endif (CMAKE_COMPILER_IS_GNUCXX)
   if (CMAKE_C_COMPILER MATCHES "icc")
      set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
      set ( CMAKE_MODULE_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_MODULE_LINKER_FLAGS}")
   endif (CMAKE_C_COMPILER MATCHES "icc")
endif (CMAKE_SYSTEM_NAME MATCHES Linux)


if (CMAKE_SYSTEM_NAME MATCHES BSD)
   set ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc")
   set ( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -lc")
endif (CMAKE_SYSTEM_NAME MATCHES BSD)


############################################################
# compiler specific settings
############################################################

ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS)
ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS)

if (MSVC)
   set (YASM_ENABLE_EXCEPTIONS -EHsc)

   # make sure that no header adds libcmt by default using #pragma comment(lib, "libcmt.lib") as done by mfc/afx.h
   #kde_check_flag_exists("/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt" CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "Release with Debug Info")
   #kde_check_flag_exists("/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt" CMAKE_EXE_LINKER_FLAGS_RELEASE "release")
   #kde_check_flag_exists("/NODEFAULTLIB:libcmt /DEFAULTLIB:msvcrt" CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "release minsize")
   #kde_check_flag_exists("/NODEFAULTLIB:libcmtd /DEFAULTLIB:msvcrtd" CMAKE_EXE_LINKER_FLAGS_DEBUG "debug")
   SET(CMAKE_CXX_WARNING_LEVEL 4 CACHE STRING "compiler warning level for CPP" FORCE)
   MARK_AS_ADVANCED(CMAKE_CXX_WARNING_LEVEL)

   # disable a few warnings that are just useless noise
   # 4146 : unary minus operator applied to unsigned type (APInt uses this)
   # 4251 : dll interface missing
   # 4275 : non-dll class used as base for dll class
   # 4800 : forcing value to bool (performance warning)
   SET(FLAGS_WARN_OFF "/wd4146 /wd4251 /wd4275 /wd4800")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS_WARN_OFF}")

   # Disable some annoying Visual Studio warnings
   ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
   ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)

   # Enable std::min and std::max
   ADD_DEFINITIONS(-DNOMINMAX)
endif(MSVC)

if (CMAKE_COMPILER_IS_GNUCXX)
   # Select flags.
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
   set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
   set(CMAKE_CXX_FLAGS_DEBUG          "-g3 -gdwarf-2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -gdwarf-2 -fno-inline")
   set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -gdwarf-2 -fno-inline -ftest-coverage -fprofile-arcs")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g3 -gdwarf-2")
   set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
   set(CMAKE_C_FLAGS_DEBUG            "-g3 -gdwarf-2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -gdwarf-2 -fno-inline")
   set(CMAKE_C_FLAGS_PROFILE          "-g3 -gdwarf-2 -fno-inline -ftest-coverage -fprofile-arcs")

   if (CMAKE_SYSTEM_NAME MATCHES Linux)
     add_definitions (-D_BSD_SOURCE)
   endif (CMAKE_SYSTEM_NAME MATCHES Linux)

   check_cxx_compiler_flag(-fPIE HAVE_FPIE_SUPPORT)
   if(YASM_ENABLE_FPIE)
       if(HAVE_FPIE_SUPPORT)
        set (YASM_CXX_FPIE_FLAGS "-fPIE")
        set (YASM_PIE_LDFLAGS "-pie")
       else(HAVE_FPIE_SUPPORT)
        message(STATUS "Your compiler doesn't support PIE flag")
       endif(HAVE_FPIE_SUPPORT)
   endif(YASM_ENABLE_FPIE)

   # visibility support
   check_cxx_compiler_flag(-fvisibility=hidden YASM_HAVE_GCC_VISIBILITY)
   set(YASM_HAVE_GCC_VISIBILITY ${YASM_HAVE_GCC_VISIBILITY} CACHE BOOL "GCC support for hidden visibility")

   # get the gcc version
   exec_program(${CMAKE_C_COMPILER} ARGS --version OUTPUT_VARIABLE _gcc_version_info)

   string (REGEX MATCH "clang version [012]" _clang_version "${_gcc_version_info}")
   if (_clang_version)
      set(_gcc_version_info "4.4.1")
   endif (_clang_version)

   string (REGEX MATCH "[345]\\.[0-9]\\.[0-9]" _gcc_version "${_gcc_version_info}")
   # gcc on mac just reports: "gcc (GCC) 3.3 20030304 ..." without the patch level, handle this here:
   if (NOT _gcc_version)
      string (REGEX REPLACE ".*\\(GCC\\).* ([34]\\.[0-9]) .*" "\\1.0" _gcc_version "${_gcc_version_info}")
   endif (NOT _gcc_version)

   macro_ensure_version("4.1.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_1)
   macro_ensure_version("4.2.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_2)
   macro_ensure_version("4.3.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_3)
   macro_ensure_version("4.4.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_4)

   # save a little by making local statics not threadsafe
   # ### do not enable it for older compilers, see
   # ### http://gcc.gnu.org/bugzilla/show_bug.cgi?id=31806
   if (GCC_IS_NEWER_THAN_4_3)
       set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")
   endif (GCC_IS_NEWER_THAN_4_3)

   set(_GCC_COMPILED_WITH_BAD_ALLOCATOR FALSE)
   if (GCC_IS_NEWER_THAN_4_1)
      exec_program(${CMAKE_C_COMPILER} ARGS -v OUTPUT_VARIABLE _gcc_alloc_info)
      string(REGEX MATCH "(--enable-libstdcxx-allocator=mt)" _GCC_COMPILED_WITH_BAD_ALLOCATOR "${_gcc_alloc_info}")
   endif (GCC_IS_NEWER_THAN_4_1)

   if (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT MINGW)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
      set (YASM_C_FLAGS "-fvisibility=hidden")

      if (GCC_IS_NEWER_THAN_4_2)
          set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
      endif (GCC_IS_NEWER_THAN_4_2)
   else (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT MINGW)
      set (YASM_HAVE_GCC_VISIBILITY 0)
   endif (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT MINGW)

    CHECK_CXX_COMPILER_FLAG(-pipe CXX_ACCEPTS_PIPE)
    IF (NOT MINGW)
        CHECK_CXX_COMPILER_FLAG(-ansi CXX_ACCEPTS_ANSI)
        CHECK_CXX_COMPILER_FLAG(-pedantic CXX_ACCEPTS_PEDANTIC)
    ENDIF (NOT MINGW)
    CHECK_CXX_COMPILER_FLAG(-Wall CXX_ACCEPTS_WALL)
    CHECK_CXX_COMPILER_FLAG(-Wextra CXX_ACCEPTS_WEXTRA)
    #CHECK_CXX_COMPILER_FLAG(-Weffc++ CXX_ACCEPTS_WEFFCPP)
    #CHECK_CXX_COMPILER_FLAG(-Wold-style-cast CXX_ACCEPTS_WOLDSTYLECAST)
    CHECK_CXX_COMPILER_FLAG(-Wstrict-null-sentinel CXX_ACCEPTS_WSTRICT_NULL)
    CHECK_CXX_COMPILER_FLAG(-Woverloaded-virtual CXX_ACCEPTS_WOVERLOADED_VIRTUAL)
    CHECK_CXX_COMPILER_FLAG(-Wno-unused-parameter CXX_ACCEPTS_WNO_UNUSED_PARAM)
    CHECK_CXX_COMPILER_FLAG(-Wnon-virtual-dtor CXX_ACCEPTS_WNON_VIRTUAL_DTOR)
    CHECK_CXX_COMPILER_FLAG(-Wno-long-long CXX_ACCEPTS_WNO_LONG_LONG)
    CHECK_CXX_COMPILER_FLAG(-Wundef CXX_ACCEPTS_WUNDEF)
    CHECK_CXX_COMPILER_FLAG(-Wcast-align CXX_ACCEPTS_WCAST_ALIGN)
    CHECK_CXX_COMPILER_FLAG(-Wchar-subscripts CXX_ACCEPTS_WCHAR_SUBSCRIPTS)
    CHECK_CXX_COMPILER_FLAG(-Wpointer-arith CXX_ACCEPTS_WPOINTER_ARITH)
    CHECK_CXX_COMPILER_FLAG(-Wformat-security CXX_ACCEPTS_WFORMAT_SECURITY)
    CHECK_CXX_COMPILER_FLAG(-fno-check-new CXX_ACCEPTS_FNO_CHECK_NEW)
    CHECK_CXX_COMPILER_FLAG(-fno-common CXX_ACCEPTS_FNO_COMMON)

    IF (CXX_ACCEPTS_PIPE)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
    ENDIF (CXX_ACCEPTS_PIPE)

    IF (CXX_ACCEPTS_ANSI)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi")
    ENDIF (CXX_ACCEPTS_ANSI)

    IF (CXX_ACCEPTS_PEDANTIC)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    ENDIF (CXX_ACCEPTS_PEDANTIC)

    IF (CXX_ACCEPTS_WALL)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    ENDIF (CXX_ACCEPTS_WALL)

    IF (CXX_ACCEPTS_WEXTRA)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    ENDIF (CXX_ACCEPTS_WEXTRA)

    #IF (CXX_ACCEPTS_WEFFCPP)
    #    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
    #ENDIF (CXX_ACCEPTS_WEFFCPP)

    #IF (CXX_ACCEPTS_WOLDSTYLECAST)
    #    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")
    #ENDIF (CXX_ACCEPTS_WOLDSTYLECAST)

    IF (CXX_ACCEPTS_WSTRICTNULL)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-null-sentinel")
    ENDIF (CXX_ACCEPTS_WSTRICTNULL)

    IF (CXX_ACCEPTS_WOVERLOADED_VIRTUAL)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
    ENDIF (CXX_ACCEPTS_WOVERLOADED_VIRTUAL)

    IF (CXX_ACCEPTS_WNO_UNUSED_PARAM)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
    ENDIF (CXX_ACCEPTS_WNO_UNUSED_PARAM)

    IF (CXX_ACCEPTS_WNON_VIRTUAL_DTOR)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
    ENDIF (CXX_ACCEPTS_WNON_VIRTUAL_DTOR)

    IF (CXX_ACCEPTS_WNO_LONG_LONG)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long")
    ENDIF (CXX_ACCEPTS_WNO_LONG_LONG)

    IF (CXX_ACCEPTS_WUNDEF)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wundef")
    ENDIF (CXX_ACCEPTS_WUNDEF)

    IF (CXX_ACCEPTS_WCAST_ALIGN)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align")
    ENDIF (CXX_ACCEPTS_WCAST_ALIGN)

    IF (CXX_ACCEPTS_WCHAR_SUBSCRIPTS)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wchar-subscripts")
    ENDIF (CXX_ACCEPTS_WCHAR_SUBSCRIPTS)

    IF (CXX_ACCEPTS_WPOINTER_ARITH)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith")
    ENDIF (CXX_ACCEPTS_WPOINTER_ARITH)

    IF (CXX_ACCEPTS_WFORMAT_SECURITY)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-security")
    ENDIF (CXX_ACCEPTS_WFORMAT_SECURITY)

    IF (CXX_ACCEPTS_FNO_CHECK_NEW)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-check-new")
    ENDIF (CXX_ACCEPTS_FNO_CHECK_NEW)

    IF (CXX_ACCEPTS_FNO_COMMON)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
    ENDIF (CXX_ACCEPTS_FNO_COMMON)

    # Work around GCC bug 36760 which causes spurious std::bind warnings.
    # This is fixed in 4.4.0

    if (GCC_IS_NEWER_THAN_4_3 AND NOT GCC_IS_NEWER_THAN_4_4)
        CHECK_CXX_COMPILER_FLAG(-Wno-ignored-qualifiers CXX_ACCEPTS_WNO_IGNORED_QUALIFIERS)
        IF (CXX_ACCEPTS_WNO_IGNORED_QUALIFIERS)
            SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers")
        ENDIF (CXX_ACCEPTS_WNO_IGNORED_QUALIFIERS)
    endif (GCC_IS_NEWER_THAN_4_3 AND NOT GCC_IS_NEWER_THAN_4_4)
endif (CMAKE_COMPILER_IS_GNUCXX)


if (CMAKE_C_COMPILER MATCHES "icc")
   # Select flags.
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
   set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
   set(CMAKE_CXX_FLAGS_DEBUG          "-O2 -g -fno-inline -noalign")
   set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g -fno-inline -noalign")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
   set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
   set(CMAKE_C_FLAGS_DEBUG            "-O2 -g -fno-inline -noalign")
   set(CMAKE_C_FLAGS_DEBUGFULL        "-g -fno-inline -noalign")

   set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -ansi -Wall -w1 -Wpointer-arith -fno-common")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -Wall -w1 -Wpointer-arith -fno-common")

   # visibility support
   set(__YASM_HAVE_ICC_VISIBILITY)
#   check_cxx_compiler_flag(-fvisibility=hidden __YASM_HAVE_ICC_VISIBILITY)
#   if (__YASM_HAVE_ICC_VISIBILITY)
#      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
#   endif (__YASM_HAVE_ICC_VISIBILITY)

endif (CMAKE_C_COMPILER MATCHES "icc")


###########    end of platform specific stuff  ##########################

# Generate yasm configured files
CONFIGURE_FILE(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

INCLUDE(YasmCXXTR1)
IF(BOOST_REQUIRED)
  FIND_PACKAGE(Boost)
  IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  ELSE(Boost_FOUND)
    MESSAGE(FATAL_ERROR "Couldn't find the Boost libraries and/or include directory to fulfill TR1 requirements.  Please install Boost or a TR1 development package. You can set BOOST_ROOT, BOOST_INCLUDEDIR and BOOST_LIBRARYDIR to help find Boost.")
  ENDIF(Boost_FOUND)
ELSE(BOOST_REQUIRED)
  INCLUDE_DIRECTORIES(${yasm_SOURCE_DIR}/lwboost)
ENDIF(BOOST_REQUIRED)

CONFIGURE_FILE(
    include/yasmx/Config/functional.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Config/functional.h
    )
CONFIGURE_FILE(
    include/yasmx/Config/export.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Config/export.h
    )
CONFIGURE_FILE(
    include/yasmx/Config/longlong.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Config/longlong.h
    )
IF(BUILD_TESTS)
    CONFIGURE_FILE(
        unittests/unittest_config.h.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/unittests/unittest_config.h
        )
ENDIF(BUILD_TESTS)

# Generate LLVM configured files
CONFIGURE_FILE(
    include/llvm/Config/config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/llvm/Config/config.h
    )
CONFIGURE_FILE(
    include/llvm/System/DataTypes.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/llvm/System/DataTypes.h
    )

