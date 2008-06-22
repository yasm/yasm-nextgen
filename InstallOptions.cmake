# Copyright (c) 2006-2008, Alexander Neundorf <neundorf@kde.org>
# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the copyright
#   notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#   derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )

if (WIN32)
# use relative install prefix to avoid hardcoded install paths in
# cmake_install.cmake files

   set(LIB_INSTALL_DIR      "lib${LIB_SUFFIX}" )            # The subdirectory relative to the install prefix where libraries will be installed (default is ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX})

   set(EXEC_INSTALL_PREFIX  "" )        # Base directory for executables and libraries
   set(SHARE_INSTALL_PREFIX "share" )   # Base directory for files which go to share/
   set(BIN_INSTALL_DIR      "bin"   )   # The install dir for executables (default ${EXEC_INSTALL_PREFIX}/bin)

   set(INCLUDE_INSTALL_DIR  "include"                     ) # The subdirectory to the header prefix

   set(PLUGIN_INSTALL_DIR       "lib${LIB_SUFFIX}/yasmx"   ) #                "The subdirectory relative to the install prefix where plugins will be installed (default is ${LIB_INSTALL_DIR}/yasmx)

   set(MAN_INSTALL_DIR           "share/man"                 ) # The man install dir (default ${SHARE_INSTALL_PREFIX}/man/)

else (WIN32)

   # This macro implements some very special logic how to deal with the cache.
   # By default the various install locations inherit their value from their "parent" variable
   # so if you set CMAKE_INSTALL_PREFIX, then EXEC_INSTALL_PREFIX, PLUGIN_INSTALL_DIR will
   # calculate their value by appending subdirs to CMAKE_INSTALL_PREFIX .
   # This would work completely without using the cache.
   # But if somebody wants e.g. a different EXEC_INSTALL_PREFIX this value has to go into
   # the cache, otherwise it will be forgotten on the next cmake run.
   # Once a variable is in the cache, it doesn't depend on its "parent" variables
   # anymore and you can only change it by editing it directly.
   # this macro helps in this regard, because as long as you don't set one of the
   # variables explicitely to some location, it will always calculate its value from its
   # parents. So modifying CMAKE_INSTALL_PREFIX later on will have the desired effect.
   # But once you decide to set e.g. EXEC_INSTALL_PREFIX to some special location
   # this will go into the cache and it will no longer depend on CMAKE_INSTALL_PREFIX.
   #
   # additionally if installing to the same location as kdelibs, the other install
   # directories are reused from the installed kdelibs
   macro(_SET_FANCY _var _value _comment)
      set(predefinedvalue "${_value}")
      if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "${YASM_INSTALL_DIR}" AND DEFINED YASM_${_var})
         set(predefinedvalue "${YASM_${_var}}")
      endif ("${CMAKE_INSTALL_PREFIX}" STREQUAL "${YASM_INSTALL_DIR}" AND DEFINED YASM_${_var})

      if (NOT DEFINED ${_var})
         set(${_var} ${predefinedvalue})
      else (NOT DEFINED ${_var})
         set(${_var} "${${_var}}" CACHE PATH "${_comment}")
      endif (NOT DEFINED ${_var})
   endmacro(_SET_FANCY)

   if(APPLE)
      set(BUNDLE_INSTALL_DIR "/Applications/Yasm" CACHE PATH "Directory where application bundles will be installed to on OSX" )
   endif(APPLE)

   _set_fancy(EXEC_INSTALL_PREFIX  "${CMAKE_INSTALL_PREFIX}"                 "Base directory for executables and libraries")
   _set_fancy(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share"           "Base directory for files which go to share/")
   _set_fancy(BIN_INSTALL_DIR      "${EXEC_INSTALL_PREFIX}/bin"              "The install dir for executables (default ${EXEC_INSTALL_PREFIX}/bin)")
   _set_fancy(LIB_INSTALL_DIR      "${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX}" "The subdirectory relative to the install prefix where libraries will be installed (default is ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX})")
   _set_fancy(INCLUDE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/include"         "The subdirectory to the header prefix")

   _set_fancy(PLUGIN_INSTALL_DIR       "${LIB_INSTALL_DIR}/yasmx"                "The subdirectory relative to the install prefix where plugins will be installed (default is ${LIB_INSTALL_DIR}/yasmx)")

   _set_fancy(MAN_INSTALL_DIR          "${SHARE_INSTALL_PREFIX}/man"            "The man install dir (default ${SHARE_INSTALL_PREFIX}/man/)")

endif (WIN32)


# The INSTALL_TARGETS_DEFAULT_ARGS variable should be used when libraries are installed.
# The arguments are also ok for regular executables, i.e. executables which don't go
# into sbin/ or libexec/, but for installing executables the basic syntax 
# INSTALL(TARGETS kate DESTINATION "${BIN_INSTALL_DIR}")
# is enough, so using this variable there doesn't help a lot.
# The variable must not be used for installing plugins.
# Usage is like this:
#    install(TARGETS kdecore kdeui ${INSTALL_TARGETS_DEFAULT_ARGS} )
#
# This will install libraries correctly under UNIX, OSX and Windows (i.e. dll's go
# into bin/.
# Later on it will be possible to extend this for installing OSX frameworks
# The COMPONENT Devel argument has the effect that static libraries belong to the 
# "Devel" install component. If we use this also for all install() commands
# for header files, it will be possible to install
#   -everything: make install OR cmake -P cmake_install.cmake
#   -only the development files: cmake -DCOMPONENT=Devel -P cmake_install.cmake
#   -everything except the development files: cmake -DCOMPONENT=Unspecified -P cmake_install.cmake
# This can then also be used for packaging with cpack.

set(INSTALL_TARGETS_DEFAULT_ARGS  RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
                                  LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
                                  ARCHIVE DESTINATION "${LIB_INSTALL_DIR}" COMPONENT Devel )

