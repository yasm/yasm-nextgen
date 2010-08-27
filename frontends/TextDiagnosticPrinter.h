//===--- TextDiagnosticPrinter.h - Text Diagnostic Client -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a concrete diagnostic client, which prints the diagnostics to
// standard error.
//
//===----------------------------------------------------------------------===//

#ifndef YASM_TEXT_DIAGNOSTIC_PRINTER_H
#define YASM_TEXT_DIAGNOSTIC_PRINTER_H

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/SourceLocation.h"

namespace yasm {
class DiagnosticOptions;

class TextDiagnosticPrinter : public DiagnosticClient {
  llvm::raw_ostream &OS;
  const DiagnosticOptions *DiagOpts;

  SourceLocation LastWarningLoc;
  FullSourceLoc LastLoc;
  unsigned LastCaretDiagnosticWasNote : 1;
  unsigned OwnsOutputStream : 1;

  /// A string to prefix to error messages.
  std::string Prefix;

public:
  TextDiagnosticPrinter(llvm::raw_ostream &os, const DiagnosticOptions &diags,
                        bool OwnsOutputStream = false);
  virtual ~TextDiagnosticPrinter();

  /// setPrefix - Set the diagnostic printer prefix string, which will be
  /// printed at the start of any diagnostics. If empty, no prefix string is
  /// used.
  void setPrefix(std::string Value) { Prefix = Value; }

  void PrintIncludeStack(SourceLocation Loc, const SourceManager &SM);

  void HighlightRange(const CharSourceRange &R,
                      const SourceManager &SrcMgr,
                      unsigned LineNo, FileID FID,
                      std::string &CaretLine,
                      const std::string &SourceLine);

  void EmitCaretDiagnostic(SourceLocation Loc,
                           CharSourceRange *Ranges, unsigned NumRanges,
                           const SourceManager &SM,
                           const FixItHint *Hints,
                           unsigned NumHints,
                           unsigned Columns,  
                           unsigned OnMacroInst,
                           unsigned MacroSkipStart,
                           unsigned MacroSkipEnd);

  virtual void HandleDiagnostic(Diagnostic::Level DiagLevel,
                                const DiagnosticInfo &Info);
};

} // end namespace yasm

#endif
