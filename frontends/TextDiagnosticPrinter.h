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

namespace llvm {
  class raw_ostream;
}

namespace yasm {
class LangOptions;
class SourceManager;

class TextDiagnosticPrinter : public DiagnosticClient {
  llvm::raw_ostream &OS;
  const LangOptions *LangOpts;
  SourceLocation LastWarningLoc;
  FullSourceLoc LastLoc;
  bool LastCaretDiagnosticWasNote;

  bool VcStyle;
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
                        bool vcStyle = false,
                        bool showColumn = true,
                        bool caretDiagnistics = true, bool showLocation = true,
                        bool printRangeInfo = true,
                        bool printDiagnosticOption = true,
                        bool printFixItInfo = true,
                        unsigned messageLength = 0,
                        bool useColors = false)
    : OS(os), LangOpts(0),
      LastCaretDiagnosticWasNote(false), VcStyle(vcStyle),
      ShowColumn(showColumn),
      CaretDiagnostics(caretDiagnistics), ShowLocation(showLocation),
      PrintRangeInfo(printRangeInfo),
      PrintDiagnosticOption(printDiagnosticOption),
      PrintFixItInfo(printFixItInfo),
      MessageLength(messageLength),
      UseColors(useColors) {}

  void setLangOptions(const LangOptions *LO) {
    LangOpts = LO;
  }

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
                           unsigned Columns);

  virtual void HandleDiagnostic(Diagnostic::Level DiagLevel,
                                const DiagnosticInfo &Info);
};

} // end namspace yasm

#endif
