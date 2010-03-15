//===-- StringRef.cpp - Lightweight String References ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/APInt.h"

using namespace llvm;

// MSVC emits references to this into the translation units which reference it.
#ifndef _MSC_VER
const size_t StringRef::npos;
#endif

static char ascii_tolower(char x) {
  if (x >= 'A' && x <= 'Z')
    return x - 'A' + 'a';
  return x;
}

/// compare_lower - Compare strings, ignoring case.
int StringRef::compare_lower(StringRef RHS) const {
  for (size_t I = 0, E = min(Length, RHS.Length); I != E; ++I) {
    char LHC = ascii_tolower(Data[I]);
    char RHC = ascii_tolower(RHS.Data[I]);
    if (LHC != RHC)
      return LHC < RHC ? -1 : 1;
  }

  if (Length == RHS.Length)
        return 0;
  return Length < RHS.Length ? -1 : 1;
}

// Compute the edit distance between the two given strings.
unsigned StringRef::edit_distance(llvm::StringRef Other, 
                                  bool AllowReplacements) {
  // The algorithm implemented below is the "classic"
  // dynamic-programming algorithm for computing the Levenshtein
  // distance, which is described here:
  //
  //   http://en.wikipedia.org/wiki/Levenshtein_distance
  //
  // Although the algorithm is typically described using an m x n
  // array, only two rows are used at a time, so this implemenation
  // just keeps two separate vectors for those two rows.
  size_type m = size();
  size_type n = Other.size();

  const unsigned SmallBufferSize = 64;
  unsigned SmallBuffer[SmallBufferSize];
  unsigned *Allocated = 0;
  unsigned *previous = SmallBuffer;
  if (2*(n + 1) > SmallBufferSize)
    Allocated = previous = new unsigned [2*(n+1)];
  unsigned *current = previous + (n + 1);
  
  for (unsigned i = 0; i <= n; ++i) 
    previous[i] = i;

  for (size_type y = 1; y <= m; ++y) {
    current[0] = y;
    for (size_type x = 1; x <= n; ++x) {
      if (AllowReplacements) {
        current[x] = min(previous[x-1] + ((*this)[y-1] == Other[x-1]? 0u:1u),
                         min(current[x-1], previous[x])+1);
      }
      else {
        if ((*this)[y-1] == Other[x-1]) current[x] = previous[x-1];
        else current[x] = min(current[x-1], previous[x]) + 1;
      }
    }
    
    unsigned *tmp = current;
    current = previous;
    previous = tmp;
  }

  unsigned Result = previous[n];
  delete [] Allocated;
  
  return Result;
}

//===----------------------------------------------------------------------===//
// String Searching
//===----------------------------------------------------------------------===//


/// find - Search for the first string \arg Str in the string.
///
/// \return - The index of the first occurence of \arg Str, or npos if not
/// found.
size_t StringRef::find(StringRef Str, size_t From) const {
  size_t N = Str.size();
  if (N > Length)
    return npos;
  for (size_t e = Length - N + 1, i = min(From, e); i != e; ++i)
    if (substr(i, N).equals(Str))
      return i;
  return npos;
}

/// rfind - Search for the last string \arg Str in the string.
///
/// \return - The index of the last occurence of \arg Str, or npos if not
/// found.
size_t StringRef::rfind(StringRef Str) const {
  size_t N = Str.size();
  if (N > Length)
    return npos;
  for (size_t i = Length - N + 1, e = 0; i != e;) {
    --i;
    if (substr(i, N).equals(Str))
      return i;
  }
  return npos;
}

/// find_first_of - Find the first character in the string that is in \arg
/// Chars, or npos if not found.
///
/// Note: O(size() * Chars.size())
StringRef::size_type StringRef::find_first_of(StringRef Chars,
                                              size_t From) const {
  for (size_type i = min(From, Length), e = Length; i != e; ++i)
    if (Chars.find(Data[i]) != npos)
      return i;
  return npos;
}

/// find_first_not_of - Find the first character in the string that is not
/// \arg C or npos if not found.
StringRef::size_type StringRef::find_first_not_of(char C, size_t From) const {
  for (size_type i = min(From, Length), e = Length; i != e; ++i)
    if (Data[i] != C)
      return i;
  return npos;
}

/// find_first_not_of - Find the first character in the string that is not
/// in the string \arg Chars, or npos if not found.
///
/// Note: O(size() * Chars.size())
StringRef::size_type StringRef::find_first_not_of(StringRef Chars,
                                                  size_t From) const {
  for (size_type i = min(From, Length), e = Length; i != e; ++i)
    if (Chars.find(Data[i]) == npos)
      return i;
  return npos;
}


//===----------------------------------------------------------------------===//
// Helpful Algorithms
//===----------------------------------------------------------------------===//

/// count - Return the number of non-overlapped occurrences of \arg Str in
/// the string.
size_t StringRef::count(StringRef Str) const {
  size_t Count = 0;
  size_t N = Str.size();
  if (N > Length)
    return 0;
  for (size_t i = 0, e = Length - N + 1; i != e; ++i)
    if (substr(i, N).equals(Str))
      ++Count;
  return Count;
}
