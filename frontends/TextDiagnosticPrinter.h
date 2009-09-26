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

#include "clang/Basic/SourceLocation.h"
#include "yasmx/Diagnostic.h"

namespace llvm {
  class raw_ostream;
}

namespace clang {
class SourceManager;
}

namespace yasm {
class LangOptions;

class TextDiagnosticPrinter : public DiagnosticClient {
  llvm::raw_ostream &OS;
  const LangOptions *LangOpts;
  clang::SourceLocation LastWarningLoc;
  clang::FullSourceLoc LastLoc;
  bool LastCaretDiagnosticWasNote;

  bool ShowColumn;
  bool CaretDiagnostics;
  bool ShowLocation;
  bool PrintRangeInfo;
  bool PrintDiagnosticOption;
  bool PrintFixItInfo;
  unsigned MessageLength;
  bool UseColors;

public:
  TextDiagnosticPrinter(llvm::raw_ostream &os,
                        bool showColumn = true,
                        bool caretDiagnistics = true, bool showLocation = true,
                        bool printRangeInfo = true,
                        bool printDiagnosticOption = true,
                        bool printFixItInfo = true,
                        unsigned messageLength = 0,
                        bool useColors = false)
    : OS(os), LangOpts(0),
      LastCaretDiagnosticWasNote(false), ShowColumn(showColumn),
      CaretDiagnostics(caretDiagnistics), ShowLocation(showLocation),
      PrintRangeInfo(printRangeInfo),
      PrintDiagnosticOption(printDiagnosticOption),
      PrintFixItInfo(printFixItInfo),
      MessageLength(messageLength),
      UseColors(useColors) {}

  void setLangOptions(const LangOptions *LO) {
    LangOpts = LO;
  }

  void PrintIncludeStack(clang::SourceLocation Loc,
                         const clang::SourceManager &SM);

  void HighlightRange(const clang::SourceRange &R,
                      const clang::SourceManager &SrcMgr,
                      unsigned LineNo, clang::FileID FID,
                      std::string &CaretLine,
                      const std::string &SourceLine);

  void EmitCaretDiagnostic(clang::SourceLocation Loc,
                           clang::SourceRange *Ranges, unsigned NumRanges,
                           clang::SourceManager &SM,
                           const CodeModificationHint *Hints,
                           unsigned NumHints,
                           unsigned Columns);

  virtual void HandleDiagnostic(Diagnostic::Level DiagLevel,
                                const DiagnosticInfo &Info);
};

} // end namspace yasm

#endif
