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

#include "llvm/ADT/OwningPtr.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"

namespace yasm {
class DiagnosticOptions;
class TextDiagnostic;

class YASM_LIB_EXPORT TextDiagnosticPrinter : public DiagnosticConsumer {
  raw_ostream &OS;
  const DiagnosticOptions *DiagOpts;

  /// \brief Handle to the currently active text diagnostic emitter.
  OwningPtr<TextDiagnostic> TextDiag;

  /// A string to prefix to error messages.
  std::string Prefix;

  unsigned OwnsOutputStream : 1;

public:
  TextDiagnosticPrinter(raw_ostream &os, const DiagnosticOptions &diags,
                        bool OwnsOutputStream = false);
  virtual ~TextDiagnosticPrinter();

  /// setPrefix - Set the diagnostic printer prefix string, which will be
  /// printed at the start of any diagnostics. If empty, no prefix string is
  /// used.
  void setPrefix(std::string Value) { Prefix = Value; }

  void BeginSourceFile();
  void EndSourceFile();
  void HandleDiagnostic(DiagnosticsEngine::Level Level, const Diagnostic &Info);
  DiagnosticConsumer *clone(DiagnosticsEngine &Diags) const;
};

} // end namespace yasm

#endif
