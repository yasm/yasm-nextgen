#ifndef YASM_PARSE_HEADERSEARCH_H
#define YASM_PARSE_HEADERSEARCH_H
//
// Resolve header file locations interface
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2010  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/DirectoryLookup.h"
#include "llvm/ADT/StringMap.h"
#include <vector>

namespace yasm
{

class FileEntry;
class FileManager;
class IdentifierInfo;

/// HeaderFileInfo - The preprocessor keeps track of this information for each
/// file that is #included.
struct HeaderFileInfo {
  /// isImport - True if this is a #import'd or #pragma once file.
  bool isImport : 1;

  /// NumIncludes - This is the number of times the file has been included
  /// already.
  unsigned short NumIncludes;

  /// ControllingMacro - If this file has a #ifndef XXX (or equivalent) guard
  /// that protects the entire contents of the file, this is the identifier
  /// for the macro that controls whether or not it has any effect.
  ///
  /// Note: Most clients should use getControllingMacro() to access
  /// the controlling macro of this header, since
  /// getControllingMacro() is able to load a controlling macro from
  /// external storage.
  const IdentifierInfo *ControllingMacro;

  HeaderFileInfo()
    : isImport(false), NumIncludes(0), ControllingMacro(0) {}

#if 0
  /// \brief Retrieve the controlling macro for this header file, if
  /// any.
  const IdentifierInfo *getControllingMacro(ExternalIdentifierLookup *External);
#endif
};

/// HeaderSearch - This class encapsulates the information needed to find the
/// file referenced by a #include or #include_next, (sub-)framework lookup, etc.
class YASM_LIB_EXPORT HeaderSearch
{
  FileManager &FileMgr;

  /// #include search path information.  Requests for #include "x" search the
  /// directory of the #including file first, then each directory in SearchDirs
  /// consequtively. Requests for <x> search the current dir first, then each
  /// directory in SearchDirs, starting at SystemDirIdx, consequtively.  If
  /// NoCurDirSearch is true, then the check for the file in the current
  /// directory is supressed.
  std::vector<DirectoryLookup> SearchDirs;
  unsigned SystemDirIdx;
  bool NoCurDirSearch;

  /// FileInfo - This contains all of the preprocessor-specific data about files
  /// that are included.  The vector is indexed by the FileEntry's UID.
  ///
  std::vector<HeaderFileInfo> FileInfo;

  /// LookupFileCache - This is keeps track of each lookup performed by
  /// LookupFile.  The first part of the value is the starting index in
  /// SearchDirs that the cached search was performed from.  If there is a hit
  /// and this value doesn't match the current query, the cache has to be
  /// ignored.  The second value is the entry in SearchDirs that satisfied the
  /// query.
  llvm::StringMap<std::pair<unsigned, unsigned> > LookupFileCache;

#if 0
  /// \brief Entity used to resolve the identifier IDs of controlling
  /// macros into IdentifierInfo pointers, as needed.
  ExternalIdentifierLookup *ExternalLookup;
#endif

  // Various statistics we track for performance analysis.
  unsigned NumIncluded;
  unsigned NumMultiIncludeFileOptzn;

  // HeaderSearch doesn't support default or copy construction.
  explicit HeaderSearch();
  explicit HeaderSearch(const HeaderSearch&);
  void operator=(const HeaderSearch&);
public:
  HeaderSearch(FileManager &FM);
  ~HeaderSearch();

  FileManager &getFileMgr() const { return FileMgr; }

  /// SetSearchPaths - Interface for setting the file search paths.
  ///
  void SetSearchPaths(const std::vector<DirectoryLookup> &dirs,
                      unsigned systemDirIdx, bool noCurDirSearch) {
    SearchDirs = dirs;
    SystemDirIdx = systemDirIdx;
    NoCurDirSearch = noCurDirSearch;
    //LookupFileCache.clear();
  }

  /// ClearFileInfo - Forget everything we know about headers so far.
  void ClearFileInfo() {
    FileInfo.clear();
  }

#if 0
  void SetExternalLookup(ExternalIdentifierLookup *EIL) {
    ExternalLookup = EIL;
  }
#endif

  /// LookupFile - Given a "foo" or <foo> reference, look up the indicated file,
  /// return null on failure.  isAngled indicates whether the file reference is
  /// a <> reference.  If successful, this returns 'UsedDir', the
  /// DirectoryLookup member the file was found in, or null if not applicable.
  /// If CurDir is non-null, the file was found in the specified directory
  /// search location.  This is used to implement #include_next.  CurFileEnt, if
  /// non-null, indicates where the #including file is, in case a relative
  /// search is needed.
  const FileEntry *LookupFile(StringRef Filename, bool isAngled,
                              const DirectoryLookup *FromDir,
                              const DirectoryLookup *&CurDir,
                              const FileEntry *CurFileEnt);

  /// ShouldEnterIncludeFile - Mark the specified file as a target of of a
  /// #include, #include_next, or #import directive.  Return false if #including
  /// the file will have no effect or true if we should include it.
  bool ShouldEnterIncludeFile(const FileEntry *File, bool isImport);


  /// MarkFileIncludeOnce - Mark the specified file as a "once only" file, e.g.
  /// due to #pragma once.
  void MarkFileIncludeOnce(const FileEntry *File) {
    getFileInfo(File).isImport = true;
  }

  /// IncrementIncludeCount - Increment the count for the number of times the
  /// specified FileEntry has been entered.
  void IncrementIncludeCount(const FileEntry *File) {
    ++getFileInfo(File).NumIncludes;
  }

  /// SetFileControllingMacro - Mark the specified file as having a controlling
  /// macro.  This is used by the multiple-include optimization to eliminate
  /// no-op #includes.
  void SetFileControllingMacro(const FileEntry *File,
                               const IdentifierInfo *ControllingMacro) {
    getFileInfo(File).ControllingMacro = ControllingMacro;
  }

  typedef std::vector<HeaderFileInfo>::iterator header_file_iterator;
  header_file_iterator header_file_begin() { return FileInfo.begin(); }
  header_file_iterator header_file_end() { return FileInfo.end(); }

  void PrintStats();
private:

  /// getFileInfo - Return the HeaderFileInfo structure for the specified
  /// FileEntry.
  HeaderFileInfo &getFileInfo(const FileEntry *FE);
};

} // namespace yasm

#endif
