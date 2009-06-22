INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(TestCXXAcceptsFlag)
INCLUDE(CMakeBackwardCompatibilityCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckLibraryExists)
INCLUDE(MacroEnsureVersion)

#FIND_PROGRAM(XMLTO NAMES xmlto)
#IF (XMLTO)
#    SET(BUILD_MAN ON)
#ENDIF (XMLTO)

# Platform-specific include files (POSIX, Win32)
CHECK_INCLUDE_FILE(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILE(libgen.h HAVE_LIBGEN_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(direct.h HAVE_DIRECT_H)

CHECK_SYMBOL_EXISTS(abort "stdlib.h" HAVE_ABORT)

CHECK_FUNCTION_EXISTS(getcwd HAVE_GETCWD)

CHECK_LIBRARY_EXISTS(dl dlopen "" HAVE_LIBDL)

IF (HAVE_LIBDL)
    SET(LIBDL "dl")
ELSE (HAVE_LIBDL)
    SET(LIBDL "")
ENDIF (HAVE_LIBDL)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
INCLUDE(FindPythonInterp)
#INCLUDE(FindPythonLibs)

#IF (PYTHONINTERP_FOUND)
#    EXEC_PROGRAM("${PYTHON_EXECUTABLE}"
#                 ARGS "${yasm_SOURCE_DIR}/CMake/have_pyrex.py"
#                 RETURN_VALUE HAVE_PYREX)
#ENDIF (PYTHONINTERP_FOUND)

FIND_PROGRAM(RE2C_EXECUTABLE NAMES re2c)

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
endif(MSVC)

if (CMAKE_COMPILER_IS_GNUCXX)
   # Select flags.
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
   set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
   set(CMAKE_CXX_FLAGS_DEBUG          "-g -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
   set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
   set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
   set(CMAKE_C_FLAGS_DEBUG            "-g -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline")
   set(CMAKE_C_FLAGS_PROFILE          "-g3 -fno-inline -ftest-coverage -fprofile-arcs")

   if (CMAKE_SYSTEM_NAME MATCHES Linux)
     add_definitions (-D_BSD_SOURCE)
   endif (CMAKE_SYSTEM_NAME MATCHES Linux)

   # gcc under Windows
   if (MINGW)
      set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols -Wl,--disable-auto-import")
      set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols -Wl,--disable-auto-import")
   endif (MINGW)

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

   if (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
      set (YASM_C_FLAGS "-fvisibility=hidden")

      if (GCC_IS_NEWER_THAN_4_2)
          set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
      endif (GCC_IS_NEWER_THAN_4_2)
   else (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR)
      set (YASM_HAVE_GCC_VISIBILITY 0)
   endif (YASM_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR)

    CHECK_CXX_COMPILER_FLAG(-pipe CXX_ACCEPTS_PIPE)
    CHECK_CXX_COMPILER_FLAG(-ansi CXX_ACCEPTS_ANSI)
    CHECK_CXX_COMPILER_FLAG(-pedantic CXX_ACCEPTS_PEDANTIC)
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

