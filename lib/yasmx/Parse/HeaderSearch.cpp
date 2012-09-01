//
// Resolve header file locations
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
#include "yasmx/Parse/HeaderSearch.h"

#include "llvm/Support/Path.h"
#include "llvm/ADT/SmallString.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Parse/IdentifierTable.h"
#include <cstdio>

using namespace yasm;

#if 0
const IdentifierInfo *
HeaderFileInfo::getControllingMacro(ExternalIdentifierLookup *External) {
  if (ControllingMacro)
    return ControllingMacro;

  if (!ControllingMacroID || !External)
    return 0;

  ControllingMacro = External->GetIdentifier(ControllingMacroID);
  return ControllingMacro;
}
#endif

HeaderSearch::HeaderSearch(FileManager &FM) : FileMgr(FM)
{
  SystemDirIdx = 0;
  NoCurDirSearch = false;

#if 0
  ExternalLookup = 0;
#endif
  NumIncluded = 0;
  NumMultiIncludeFileOptzn = 0;
}

HeaderSearch::~HeaderSearch()
{
}

void HeaderSearch::PrintStats()
{
  fprintf(stderr, "\n*** HeaderSearch Stats:\n");
  fprintf(stderr, "%d files tracked.\n", (int)FileInfo.size());
  unsigned NumOnceOnlyFiles = 0, MaxNumIncludes = 0, NumSingleIncludedFiles = 0;
  for (unsigned i = 0, e = FileInfo.size(); i != e; ++i) {
    NumOnceOnlyFiles += FileInfo[i].isImport;
    if (MaxNumIncludes < FileInfo[i].NumIncludes)
      MaxNumIncludes = FileInfo[i].NumIncludes;
    NumSingleIncludedFiles += FileInfo[i].NumIncludes == 1;
  }
  fprintf(stderr, "  %d #import/#pragma once files.\n", NumOnceOnlyFiles);
  fprintf(stderr, "  %d included exactly once.\n", NumSingleIncludedFiles);
  fprintf(stderr, "  %d max times a file is included.\n", MaxNumIncludes);

  fprintf(stderr, "  %d #include/#include_next/#import.\n", NumIncluded);
  fprintf(stderr, "    %d #includes skipped due to"
          " the multi-include optimization.\n", NumMultiIncludeFileOptzn);
}

//===----------------------------------------------------------------------===//
// File lookup within a DirectoryLookup scope
//===----------------------------------------------------------------------===//

/// getName - Return the directory or filename corresponding to this lookup
/// object.
const char *DirectoryLookup::getName() const
{
  return getDir()->getName();
}


/// LookupFile - Lookup the specified file in this search path, returning it
/// if it exists or returning null if not.
const FileEntry *DirectoryLookup::LookupFile(StringRef Filename,
                                             HeaderSearch &HS) const
{
  SmallString<1024> TmpDir;
  // Concatenate the requested file onto the directory.
  // FIXME: Portability.  Filename concatenation should be in sys::Path.
  TmpDir += getDir()->getName();
  TmpDir.push_back('/');
  TmpDir.append(Filename.begin(), Filename.end());
  return HS.getFileMgr().getFile(TmpDir);
}


//===----------------------------------------------------------------------===//
// Header File Location.
//===----------------------------------------------------------------------===//


/// LookupFile - Given a "foo" or <foo> reference, look up the indicated file,
/// return null on failure.  isAngled indicates whether the file reference is
/// for system #include's or not (i.e. using <> instead of "").  CurFileEnt, if
/// non-null, indicates where the #including file is, in case a relative search
/// is needed.
const FileEntry *
HeaderSearch::LookupFile(StringRef Filename,
                         bool isAngled,
                         const DirectoryLookup *FromDir,
                         const DirectoryLookup *&CurDir,
                         const FileEntry *CurFileEnt)
{
  // If 'Filename' is absolute, check to see if it exists and no searching.
  if (llvm::sys::path::is_absolute(Filename)) {
    CurDir = 0;

    // If this was an #include_next "/absolute/file", fail.
    if (FromDir) return 0;

    // Otherwise, just return the file.
    return FileMgr.getFile(Filename);
  }

  // Step #0, unless disabled, check to see if the file is in the #includer's
  // directory.  This has to be based on CurFileEnt, not CurDir, because
  // CurFileEnt could be a #include of a subdirectory (#include "foo/bar.h") and
  // a subsequent include of "baz.h" should resolve to "whatever/foo/baz.h".
  // This search is not done for <> headers.
  if (CurFileEnt && !isAngled && !NoCurDirSearch) {
    SmallString<1024> TmpDir;
    // Concatenate the requested file onto the directory.
    // FIXME: Portability.  Filename concatenation should be in sys::Path.
    TmpDir += CurFileEnt->getDir()->getName();
    TmpDir.push_back('/');
    TmpDir.append(Filename.begin(), Filename.end());
    if (const FileEntry *FE = FileMgr.getFile(TmpDir)) {
      // Leave CurDir unset.
      return FE;
    }
  }

  CurDir = 0;

  // If this is a system #include, ignore the user #include locs.
  unsigned i = isAngled ? SystemDirIdx : 0;

  // If this is a #include_next request, start searching after the directory the
  // file was found in.
  if (FromDir)
    i = FromDir-&SearchDirs[0];

  // Cache all of the lookups performed by this method.  Many headers are
  // multiply included, and the "pragma once" optimization prevents them from
  // being relex/pp'd, but they would still have to search through a
  // (potentially huge) series of SearchDirs to find it.
  std::pair<unsigned, unsigned> &CacheLookup =
    LookupFileCache.GetOrCreateValue(Filename).getValue();

  // If the entry has been previously looked up, the first value will be
  // non-zero.  If the value is equal to i (the start point of our search), then
  // this is a matching hit.
  if (CacheLookup.first == i+1) {
    // Skip querying potentially lots of directories for this lookup.
    i = CacheLookup.second;
  } else {
    // Otherwise, this is the first query, or the previous query didn't match
    // our search start.  We will fill in our found location below, so prime the
    // start point value.
    CacheLookup.first = i+1;
  }

  // Check each directory in sequence to see if it contains this file.
  for (; i != SearchDirs.size(); ++i) {
    const FileEntry *FE =
      SearchDirs[i].LookupFile(Filename, *this);
    if (!FE) continue;

    CurDir = &SearchDirs[i];

    // Remember this location for the next lookup we do.
    CacheLookup.second = i;
    return FE;
  }

  // Otherwise, didn't find it. Remember we didn't find this.
  CacheLookup.second = SearchDirs.size();
  return 0;
}

//===----------------------------------------------------------------------===//
// File Info Management.
//===----------------------------------------------------------------------===//


/// getFileInfo - Return the HeaderFileInfo structure for the specified
/// FileEntry.
HeaderFileInfo &HeaderSearch::getFileInfo(const FileEntry *FE) {
  if (FE->getUID() >= FileInfo.size())
    FileInfo.resize(FE->getUID()+1);
  return FileInfo[FE->getUID()];
}

/// ShouldEnterIncludeFile - Mark the specified file as a target of of a
/// #include, #include_next, or #import directive.  Return false if #including
/// the file will have no effect or true if we should include it.
bool
HeaderSearch::ShouldEnterIncludeFile(const FileEntry *File, bool isImport)
{
  ++NumIncluded; // Count # of attempted #includes.

  // Get information about this file.
  HeaderFileInfo &FileInfo = getFileInfo(File);

  // If this is a #import directive, check that we have not already imported
  // this header.
  if (isImport) {
    // If this has already been imported, don't import it again.
    FileInfo.isImport = true;

    // Has this already been #import'ed or #include'd?
    if (FileInfo.NumIncludes) return false;
  } else {
    // Otherwise, if this is a #include of a file that was previously #import'd
    // or if this is the second #include of a #pragma once file, ignore it.
    if (FileInfo.isImport)
      return false;
  }

#if 0
  // Next, check to see if the file is wrapped with #ifndef guards.  If so, and
  // if the macro that guards it is defined, we know the #include has no effect.
  if (const IdentifierInfo *ControllingMacro
      = FileInfo.getControllingMacro(ExternalLookup))
    if (ControllingMacro->hasMacroDefinition()) {
      ++NumMultiIncludeFileOptzn;
      return false;
    }
#endif

  // Increment the number of times this file has been included.
  ++FileInfo.NumIncludes;

  return true;
}


