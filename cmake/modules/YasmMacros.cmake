# Portions based on kdelibs KDE4Macros.cmake:
#
# Copyright (c) 2006, 2007, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2006, 2007, Laurent Montel, <montel@kde.org>
# Copyright (c) 2007 Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# Changes for Yasm Copyright (c) 2007 Peter Johnson

INCLUDE(MacroAdditionalCleanFiles)

# This macro sets the RPATH related options for libraries, plugins and
# executables.
# If RPATH is not explicitly disabled, libraries and plugins are built without
# RPATH, in the hope that the RPATH which is compiled into the executable is
# good enough.
macro (YASM_HANDLE_RPATH_FOR_LIBRARY _target_NAME)
   if (NOT CMAKE_SKIP_RPATH)
      if(YASM_USE_ALWAYS_FULL_RPATH)
         set_target_properties(${_target_NAME} PROPERTIES
                               SKIP_BUILD_RPATH FALSE
                               BUILD_WITH_INSTALL_RPATH FALSE)
      else(YASM_USE_ALWAYS_FULL_RPATH)
         set_target_properties(${_target_NAME} PROPERTIES
                               INSTALL_RPATH_USE_LINK_PATH FALSE
                               SKIP_BUILD_RPATH TRUE
                               BUILD_WITH_INSTALL_RPATH TRUE
                               INSTALL_RPATH "")
      endif(YASM_USE_ALWAYS_FULL_RPATH)
   endif (NOT CMAKE_SKIP_RPATH)
endmacro (YASM_HANDLE_RPATH_FOR_LIBRARY)

# Executables with the RUN_UNINSTALLED option will be built with the RPATH
# pointing to the build dir, so that they can be run safely without being
# installed, e.g. as code generators for other stuff during the build. These
# executables will be relinked during "make install".
# All other executables are built with the RPATH with which they will be
# installed.
macro (YASM_HANDLE_RPATH_FOR_EXECUTABLE _target_NAME _type)
   if (UNIX)
      # set the RPATH related properties
      if (NOT CMAKE_SKIP_RPATH)
         if (${_type} STREQUAL "NORMAL")
            set_target_properties(${_target_NAME} PROPERTIES
                                  SKIP_BUILD_RPATH TRUE
                                  BUILD_WITH_INSTALL_RPATH TRUE)
         endif (${_type} STREQUAL "NORMAL")

         if (${_type} STREQUAL "RUN_UNINSTALLED")
            set_target_properties(${_target_NAME} PROPERTIES
                                  SKIP_BUILD_RPATH FALSE
                                  BUILD_WITH_INSTALL_RPATH FALSE)
         endif (${_type} STREQUAL "RUN_UNINSTALLED")
      endif (NOT CMAKE_SKIP_RPATH)
   endif (UNIX)
endmacro (YASM_HANDLE_RPATH_FOR_EXECUTABLE)


macro (YASM_ADD_PLUGIN _target_NAME _with_PREFIX)
# is the first argument is "WITH_PREFIX" then keep the standard "lib" prefix,
# otherwise set the prefix empty
   if (${_with_PREFIX} STREQUAL "WITH_PREFIX")
      set(_first_SRC)
   else (${_with_PREFIX} STREQUAL "WITH_PREFIX")
      set(_first_SRC ${_with_PREFIX})
   endif (${_with_PREFIX} STREQUAL "WITH_PREFIX")

   set(_SRCS ${_first_SRC} ${ARGN})
   if (BUILD_STATIC)
       add_library(${_target_NAME} STATIC ${_SRCS})
   else (BUILD_STATIC)
       add_library(${_target_NAME} MODULE ${_SRCS})
       if (_first_SRC)
           set_target_properties(${_target_NAME} PROPERTIES PREFIX "")
       endif (_first_SRC)
   endif (BUILD_STATIC)


   if (NOT BUILD_STATIC)
       yasm_handle_rpath_for_library(${_target_NAME})
   endif (NOT BUILD_STATIC)

   if (WIN32)
      # for shared libraries/plugins a -DMAKE_target_LIB is required
      string(TOUPPER ${_target_NAME} _symbol)
      string(REGEX REPLACE "[^_A-Za-z0-9]" "_" _symbol ${_symbol})
      set(_symbol "MAKE_${_symbol}_LIB")
      set_target_properties(${_target_NAME} PROPERTIES DEFINE_SYMBOL ${_symbol})
   endif (WIN32)

endmacro (YASM_ADD_PLUGIN _target_NAME _with_PREFIX)


# this macro is intended to check whether a list of source
# files has the "RUN_UNINSTALLED" keyword at the beginning
# in _output_LIST the list of source files is returned with the
# "RUN_UNINSTALLED" keyword removed
# if "RUN_UNINSTALLED" is in the list of files, the _uninst argument is set to
# "RUN_UNINSTALLED" (which evaluates to TRUE in cmake), otherwise it is set
# empty (which evaluates to FALSE in cmake)
# if "TEST" is in the list of files, the _test argument is set to
# "TEST" (which evaluates to TRUE in cmake), otherwise it is set empty
# (which evaluates to FALSE in cmake)
macro(YASM_CHECK_EXECUTABLE_PARAMS _output_LIST _uninst _test)
   set(${_uninst})
   set(${_test})
   set(${_output_LIST} ${ARGN})
   list(LENGTH ${_output_LIST} count)

   list(GET ${_output_LIST} 0 first_PARAM)

   set(second_PARAM "NOTFOUND")
   if (${count} GREATER 1)
      list(GET ${_output_LIST} 1 second_PARAM)
   endif (${count} GREATER 1)

   set(remove "NOTFOUND")

   if (${first_PARAM} STREQUAL "RUN_UNINSTALLED")
      set(${_uninst} "RUN_UNINSTALLED")
      set(remove 0)
   endif (${first_PARAM} STREQUAL "RUN_UNINSTALLED")

   if (${first_PARAM} STREQUAL "TEST")
      set(${_test} "TEST")
      set(remove 0)
   endif (${first_PARAM} STREQUAL "TEST")

   if (${second_PARAM} STREQUAL "RUN_UNINSTALLED")
      set(${_uninst} "RUN_UNINSTALLED")
      set(remove 0;1)
   endif (${second_PARAM} STREQUAL "RUN_UNINSTALLED")

   if (${second_PARAM} STREQUAL "TEST")
      set(${_test} "TEST")
      set(remove 0;1)
   endif (${second_PARAM} STREQUAL "TEST")

   if (NOT "${remove}" STREQUAL "NOTFOUND")
      list(REMOVE_AT ${_output_LIST} ${remove})
   endif (NOT "${remove}" STREQUAL "NOTFOUND")

endmacro(YASM_CHECK_EXECUTABLE_PARAMS)

# add a unit test, which is executed when running make test
# it will be built with RPATH pointing to the build dir
# The targets are always created, but only built for the "all"
# target if the option BUILD_TESTING is enabled. Otherwise the rules for
# the target are created but not built by default. You can build them by
# manually building the target.
# The name of the target can be specified using TESTNAME <testname>, if it is
# not given the macro will default to the <name>
macro (YASM_ADD_UNIT_TEST _test_NAME)
    set(_srcList ${ARGN})
    set(_targetName ${_test_NAME})
    if( ${ARGV1} STREQUAL "TESTNAME" )
        set(_targetName ${ARGV2})
        LIST(REMOVE_AT _srcList 0 1)
    endif( ${ARGV1} STREQUAL "TESTNAME" )
    yasm_add_executable( ${_test_NAME} TEST ${_srcList} )
    set(_executable ${_test_NAME})
    add_test( ${_targetName} ${_executable} )
endmacro (YASM_ADD_UNIT_TEST)

# add an executable
# it will be built with RPATH pointing to the build dir
# The targets are always created, but only built for the "all"
# target if the option BUILD_TESTING is enabled. Otherwise the rules for
# the target are created but not built by default. You can build them by
# manually building the target.
macro (YASM_ADD_EXECUTABLE _target_NAME)

   yasm_check_executable_params( _SRCS _uninst _test ${ARGN})

   set(_type "NORMAL")
   if (_uninst OR _test)
      set(_type "RUN_UNINSTALLED")
   endif (_uninst OR _test)

   set(_add_executable_param)
   if (_test AND NOT BUILD_TESTING)
      set(_add_executable_param ${_add_executable_param} EXCLUDE_FROM_ALL)
   endif (_test AND NOT BUILD_TESTING)

   add_executable(${_target_NAME} ${_add_executable_param} ${_SRCS})
   if (NOT BUILD_STATIC)
       yasm_handle_rpath_for_executable(${_target_NAME} ${_type})
   endif (NOT BUILD_STATIC)
   TARGET_LINK_LIBRARIES(${_target_NAME} yasmstdx libyasmx)

endmacro (YASM_ADD_EXECUTABLE)

macro (YASM_ADD_LIBRARY _target_NAME _lib_TYPE)
   set(_first_SRC ${_lib_TYPE})
   set(_add_lib_param)

   if (${_lib_TYPE} STREQUAL "STATIC")
      set(_first_SRC)
      set(_add_lib_param STATIC)
   endif (${_lib_TYPE} STREQUAL "STATIC")
   if (${_lib_TYPE} STREQUAL "SHARED")
      set(_first_SRC)
      set(_add_lib_param SHARED)
   endif (${_lib_TYPE} STREQUAL "SHARED")
   if (${_lib_TYPE} STREQUAL "MODULE")
      set(_first_SRC)
      set(_add_lib_param MODULE)
   endif (${_lib_TYPE} STREQUAL "MODULE")

   if (BUILD_STATIC)
       set(_add_lib_param STATIC)
   endif (BUILD_STATIC)

   set(_SRCS ${_first_SRC} ${ARGN})
   add_library(${_target_NAME} ${_add_lib_param} ${_SRCS})

   if (NOT BUILD_STATIC)
       yasm_handle_rpath_for_library(${_target_NAME})
   endif (NOT BUILD_STATIC)

   # for shared libraries a -DMAKE_target_LIB is required
   string(TOUPPER ${_target_NAME} _symbol)
   string(REGEX REPLACE "[^_A-Za-z0-9]" "_" _symbol ${_symbol})
   set(_symbol "MAKE_${_symbol}_LIB")
   set_target_properties(${_target_NAME} PROPERTIES DEFINE_SYMBOL ${_symbol})

endmacro (YASM_ADD_LIBRARY _target_NAME _lib_TYPE)

macro (YASM_ADD_MODULE _module_NAME)
    list(APPEND YASM_MODULES_SRC ${ARGN})
    list(APPEND YASM_MODULES ${_module_NAME})
endmacro (YASM_ADD_MODULE)

macro (YASM_GENPERF _in_NAME _out_NAME)
    get_target_property(_tmp_GENPERF_EXE genperf LOCATION)
    add_custom_command(
        OUTPUT ${_out_NAME}
        COMMAND ${_tmp_GENPERF_EXE} ${_in_NAME} ${_out_NAME}
        DEPENDS ${_tmp_GENPERF_EXE}
        MAIN_DEPENDENCY ${_in_NAME}
        )
endmacro (YASM_GENPERF)
