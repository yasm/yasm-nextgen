///
/// @file libyasm/plugin.cpp
/// @brief YASM plugin loader implementation for Windows and Unix.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/System/plugin.h"

#include <string>
#include <vector>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#elif defined(__MACH__) || defined(__APPLE__) || defined(__GNUC__)
#include <dlfcn.h>
#endif

#include "config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "llvm/ADT/StringRef.h"


using namespace yasm;

static std::vector<void*> loaded_plugins;

static void*
LoadDLL(const std::string& name)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    return LoadLibrary(name.c_str());
#elif defined(__MACH__) || defined(__APPLE__) || defined(__GNUC__)
    return dlopen(name.c_str(), RTLD_NOW);
#else
    return 0;
#endif
}

bool
yasm::LoadPlugin(StringRef name)
{
    // Load library
    void* lib = 0;

    // First attempt: FOO.so
    std::string path = name;
#if defined(_MSC_VER) || defined(__MINGW32__)
    if (!name.endswith(".dll"))
        path += ".dll";
#elif defined(__MACH__) || defined(__APPLE__)
    if (!name.endswith(".dylib"))
        path += ".dylib";
#elif defined(__GNUC__)
    if (!name.endswith(".so") != 0)
        path += ".so";
#endif
    lib = LoadDLL(path);

    // Second attempt: PLUGIN_INSTALL_DIR/FOO.so
    if (!lib && path.find_first_of("\\/") == std::string::npos)
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
        path.insert(0, PLUGIN_INSTALL_DIR "\\");
#elif defined(__GNUC__)
        path.insert(0, PLUGIN_INSTALL_DIR "/");
#endif
        lib = LoadDLL(path);
    }

    // Last attempt: FOO
    if (!lib)
        lib = LoadDLL(name);

    if (!lib)
        return false;   // Didn't load successfully

    // Add to vector of loaded plugins
    loaded_plugins.push_back(lib);

    // Get yasm_init_plugin() function and run it

    void (*init_plugin) (void) = 0;
#if defined(_MSC_VER) || defined(__MINGW32__)
    init_plugin = reinterpret_cast<void (*)(void)>
        (GetProcAddress((HINSTANCE)lib, "yasm_init_plugin"));
#elif defined(__MACH__) || defined(__APPLE__) || defined(__GNUC__)
    init_plugin = reinterpret_cast<void (*)(void)>
        (reinterpret_cast<uintptr_t>(dlsym(lib, "yasm_init_plugin")));
#endif

    if (!init_plugin)
        return false;   // Didn't load successfully

    init_plugin();
    return true;
}

void
yasm::UnloadPlugins(void)
{
    while (!loaded_plugins.empty()) {
        void* plugin = loaded_plugins.back();
        loaded_plugins.pop_back();
#if defined(_MSC_VER) || defined(__MINGW32__)
        FreeLibrary(reinterpret_cast<HINSTANCE>(plugin));
#elif defined(__MACH__) || defined(__APPLE__) || defined(__GNUC__)
        dlclose(plugin);
#endif
    }
}
