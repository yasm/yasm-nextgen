bin_PROGRAMS = ygas
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
noinst_PROGRAMS =

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Iinclude -I$(srcdir)/include -I$(srcdir)/lwboost
AM_CPPFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
AM_CPPFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE

ygas_SOURCES  = frontends/ygas.cpp
nodist_ygas_SOURCES =
ygas_SOURCES += frontends/DiagnosticOptions.h
ygas_SOURCES += frontends/TextDiagnosticPrinter.cpp
ygas_SOURCES += frontends/TextDiagnosticPrinter.h

$(srcdir)/frontends/ygas.cpp: license.cpp
license.cpp: $(srcdir)/Copying/COPYING $(srcdir)/frontends/genstring.py
	$(PYTHON) $(srcdir)/frontends/genstring.py license_msg license.cpp \
	    $(srcdir)/Copying/COPYING
BUILT_SOURCES += license.cpp
CLEANFILES += license.cpp
EXTRA_DIST += frontends/genstring.py

# diagnostic includes
lib/yasmx/Basic/DiagnosticGroups.cpp lib/yasmx/Basic/DiagnosticCategories.cpp include/yasmx/Basic/DiagnosticKinds.h lib/yasmx/Basic/StaticDiagInfo.inc: $(srcdir)/include/yasmx/Basic/gen_diag.py
	$(MKDIR_P) lib/yasmx/Basic
	$(MKDIR_P) include/yasmx/Basic
	$(PYTHON) $(srcdir)/include/yasmx/Basic/gen_diag.py \
            lib/yasmx/Basic/DiagnosticGroups.cpp \
            lib/yasmx/Basic/DiagnosticCategories.cpp \
            include/yasmx/Basic/DiagnosticKinds.h \
            lib/yasmx/Basic/StaticDiagInfo.inc
BUILT_SOURCES += lib/yasmx/Basic/DiagnosticGroups.cpp
BUILT_SOURCES += lib/yasmx/Basic/DiagnosticCategories.cpp
BUILT_SOURCES += include/yasmx/Basic/DiagnosticKinds.h
BUILT_SOURCES += lib/yasmx/Basic/StaticDiagInfo.inc
CLEANFILES += lib/yasmx/Basic/DiagnosticGroups.cpp
CLEANFILES += lib/yasmx/Basic/DiagnosticCategories.cpp
CLEANFILES += include/yasmx/Basic/DiagnosticKinds.h
CLEANFILES += lib/yasmx/Basic/StaticDiagInfo.inc
EXTRA_DIST += include/yasmx/Basic/gen_diag.py

clean-local:
	-rmdir lib/yasmx/Basic
	-rmdir lib/yasmx
	-rmdir lib
	-rmdir include/yasmx/Basic
	-rmdir include/yasmx
	-rmdir include

ygas_SOURCES += include/llvm/ADT/APInt.h
ygas_SOURCES += include/llvm/ADT/APFloat.h
ygas_SOURCES += include/llvm/ADT/DenseMap.h
ygas_SOURCES += include/llvm/ADT/DenseMapInfo.h
ygas_SOURCES += include/llvm/ADT/IndexedMap.h
ygas_SOURCES += include/llvm/ADT/IntrusiveRefCntPtr.h
ygas_SOURCES += include/llvm/ADT/OwningPtr.h
ygas_SOURCES += include/llvm/ADT/PointerIntPair.h
ygas_SOURCES += include/llvm/ADT/PointerUnion.h
ygas_SOURCES += include/llvm/ADT/SmallPtrSet.h
ygas_SOURCES += include/llvm/ADT/SmallString.h
ygas_SOURCES += include/llvm/ADT/SmallVector.h
ygas_SOURCES += include/llvm/ADT/STLExtras.h
ygas_SOURCES += include/llvm/ADT/StringExtras.h
ygas_SOURCES += include/llvm/ADT/StringMap.h
ygas_SOURCES += include/llvm/ADT/StringRef.h
ygas_SOURCES += include/llvm/ADT/Twine.h
ygas_SOURCES += include/llvm/Support/AlignOf.h
ygas_SOURCES += include/llvm/Support/Allocator.h
ygas_SOURCES += include/llvm/Support/Casting.h
ygas_SOURCES += include/llvm/Support/Compiler.h
ygas_SOURCES += include/llvm/Support/Debug.h
ygas_SOURCES += include/llvm/Support/ErrorHandling.h
ygas_SOURCES += include/llvm/Support/Format.h
ygas_SOURCES += include/llvm/Support/MathExtras.h
ygas_SOURCES += include/llvm/Support/MemoryBuffer.h
ygas_SOURCES += include/llvm/Support/PointerLikeTypeTraits.h
ygas_SOURCES += include/llvm/Support/raw_ostream.h
ygas_SOURCES += include/llvm/Support/type_traits.h
ygas_SOURCES += include/llvm/System/AIXDataTypesFix.h
ygas_SOURCES += include/llvm/System/DataTypes.h
ygas_SOURCES += include/llvm/System/Errno.h
ygas_SOURCES += include/llvm/System/Memory.h
ygas_SOURCES += include/llvm/System/Path.h
ygas_SOURCES += include/llvm/System/Process.h
ygas_SOURCES += include/llvm/System/TimeValue.h
ygas_SOURCES += include/yasmx/Arch.h
ygas_SOURCES += include/yasmx/Assembler.h
ygas_SOURCES += include/yasmx/AssocData.h
ygas_SOURCES += include/yasmx/Bytecode.h
ygas_SOURCES += include/yasmx/BytecodeContainer.h
ygas_SOURCES += include/yasmx/BytecodeOutput.h
ygas_SOURCES += include/yasmx/Bytes.h
ygas_SOURCES += include/yasmx/Bytes_leb128.h
ygas_SOURCES += include/yasmx/Bytes_util.h
ygas_SOURCES += include/yasmx/DebugFormat.h
ygas_SOURCES += include/yasmx/EffAddr.h
ygas_SOURCES += include/yasmx/Expr.h
ygas_SOURCES += include/yasmx/Expr_util.h
ygas_SOURCES += include/yasmx/InputBuffer.h
ygas_SOURCES += include/yasmx/Insn.h
ygas_SOURCES += include/yasmx/IntNum.h
ygas_SOURCES += include/yasmx/ListFormat.h
ygas_SOURCES += include/yasmx/Location.h
ygas_SOURCES += include/yasmx/Location_util.h
ygas_SOURCES += include/yasmx/Module.h
ygas_SOURCES += include/yasmx/NumericOutput.h
ygas_SOURCES += include/yasmx/Object.h
ygas_SOURCES += include/yasmx/Object_util.h
ygas_SOURCES += include/yasmx/ObjectFormat.h
ygas_SOURCES += include/yasmx/Op.h
ygas_SOURCES += include/yasmx/Reloc.h
ygas_SOURCES += include/yasmx/Section.h
ygas_SOURCES += include/yasmx/StringTable.h
ygas_SOURCES += include/yasmx/Symbol.h
ygas_SOURCES += include/yasmx/SymbolRef.h
ygas_SOURCES += include/yasmx/Symbol_util.h
ygas_SOURCES += include/yasmx/Value.h
ygas_SOURCES += include/yasmx/Basic/Diagnostic.h
ygas_SOURCES += include/yasmx/Basic/FileManager.h
ygas_SOURCES += include/yasmx/Basic/PartialDiagnostic.h
ygas_SOURCES += include/yasmx/Basic/PrettyStackTrace.h
ygas_SOURCES += include/yasmx/Basic/SourceLocation.h
ygas_SOURCES += include/yasmx/Basic/SourceManager.h
ygas_SOURCES += include/yasmx/Basic/SourceManagerInternals.h
ygas_SOURCES += include/yasmx/Config/export.h
ygas_SOURCES += include/yasmx/Config/longlong.h
ygas_SOURCES += include/yasmx/Parse/Directive.h
ygas_SOURCES += include/yasmx/Parse/DirectoryLookup.h
ygas_SOURCES += include/yasmx/Parse/DirHelpers.h
ygas_SOURCES += include/yasmx/Parse/HeaderSearch.h
ygas_SOURCES += include/yasmx/Parse/IdentifierTable.h
ygas_SOURCES += include/yasmx/Parse/Lexer.h
ygas_SOURCES += include/yasmx/Parse/NameValue.h
ygas_SOURCES += include/yasmx/Parse/NumericParser.h
ygas_SOURCES += include/yasmx/Parse/Parser.h
ygas_SOURCES += include/yasmx/Parse/ParserImpl.h
ygas_SOURCES += include/yasmx/Parse/Preprocessor.h
ygas_SOURCES += include/yasmx/Parse/Token.h
ygas_SOURCES += include/yasmx/Parse/TokenLexer.h
ygas_SOURCES += include/yasmx/Support/bitcount.h
ygas_SOURCES += include/yasmx/Support/EndianState.h
ygas_SOURCES += include/yasmx/Support/IntervalTree.h
ygas_SOURCES += include/yasmx/Support/MD5.h
ygas_SOURCES += include/yasmx/Support/phash.h
ygas_SOURCES += include/yasmx/Support/ptr_vector.h
ygas_SOURCES += include/yasmx/Support/registry.h
ygas_SOURCES += include/yasmx/Support/scoped_array.h
ygas_SOURCES += include/yasmx/Support/scoped_ptr.h
ygas_SOURCES += include/yasmx/System/plugin.h

ygas_SOURCES += lib/llvm/Support/Allocator.cpp
ygas_SOURCES += lib/llvm/Support/APFloat.cpp
ygas_SOURCES += lib/llvm/Support/APInt.cpp
ygas_SOURCES += lib/llvm/Support/ErrorHandling.cpp
ygas_SOURCES += lib/llvm/Support/raw_ostream.cpp
ygas_SOURCES += lib/llvm/Support/MemoryBuffer.cpp
ygas_SOURCES += lib/llvm/Support/SmallPtrSet.cpp
ygas_SOURCES += lib/llvm/Support/SmallVector.cpp
ygas_SOURCES += lib/llvm/Support/StringExtras.cpp
ygas_SOURCES += lib/llvm/Support/StringMap.cpp
ygas_SOURCES += lib/llvm/Support/StringRef.cpp
ygas_SOURCES += lib/llvm/Support/Twine.cpp
ygas_SOURCES += lib/llvm/System/Errno.cpp
ygas_SOURCES += lib/llvm/System/Memory.cpp
ygas_SOURCES += lib/llvm/System/Path.cpp
ygas_SOURCES += lib/llvm/System/Process.cpp
ygas_SOURCES += lib/llvm/System/TimeValue.cpp
ygas_SOURCES += lib/yasmx/Basic/Diagnostic.cpp
ygas_SOURCES += lib/yasmx/Basic/FileManager.cpp
ygas_SOURCES += lib/yasmx/Basic/SourceLocation.cpp
ygas_SOURCES += lib/yasmx/Basic/SourceManager.cpp
ygas_SOURCES += lib/yasmx/Parse/Directive.cpp
ygas_SOURCES += lib/yasmx/Parse/DirHelpers.cpp
ygas_SOURCES += lib/yasmx/Parse/HeaderSearch.cpp
ygas_SOURCES += lib/yasmx/Parse/IdentifierTable.cpp
ygas_SOURCES += lib/yasmx/Parse/Lexer.cpp
ygas_SOURCES += lib/yasmx/Parse/NameValue.cpp
ygas_SOURCES += lib/yasmx/Parse/NumericParser.cpp
ygas_SOURCES += lib/yasmx/Parse/Parser.cpp
ygas_SOURCES += lib/yasmx/Parse/ParserImpl.cpp
ygas_SOURCES += lib/yasmx/Parse/Preprocessor.cpp
ygas_SOURCES += lib/yasmx/Parse/PPCaching.cpp
ygas_SOURCES += lib/yasmx/Parse/PPLexerChange.cpp
ygas_SOURCES += lib/yasmx/Parse/TokenLexer.cpp
ygas_SOURCES += lib/yasmx/Support/MD5.cpp
ygas_SOURCES += lib/yasmx/Support/phash.cpp
ygas_SOURCES += lib/yasmx/Support/registry.cpp
ygas_SOURCES += lib/yasmx/AlignBytecode.cpp
ygas_SOURCES += lib/yasmx/Arch.cpp
ygas_SOURCES += lib/yasmx/Assembler.cpp
ygas_SOURCES += lib/yasmx/AssocData.cpp
ygas_SOURCES += lib/yasmx/BytecodeContainer.cpp
ygas_SOURCES += lib/yasmx/BytecodeOutput.cpp
ygas_SOURCES += lib/yasmx/Bytecode.cpp
ygas_SOURCES += lib/yasmx/Bytes.cpp
ygas_SOURCES += lib/yasmx/Bytes_util.cpp
ygas_SOURCES += lib/yasmx/Bytes_leb128.cpp
ygas_SOURCES += lib/yasmx/DataBytecode.cpp
ygas_SOURCES += lib/yasmx/DebugFormat.cpp
ygas_SOURCES += lib/yasmx/EffAddr.cpp
ygas_SOURCES += lib/yasmx/Expr.cpp
ygas_SOURCES += lib/yasmx/Expr_util.cpp
ygas_SOURCES += lib/yasmx/hamt.h
ygas_SOURCES += lib/yasmx/IncbinBytecode.cpp
ygas_SOURCES += lib/yasmx/InputBuffer.cpp
ygas_SOURCES += lib/yasmx/Insn.cpp
ygas_SOURCES += lib/yasmx/IntNum.cpp
ygas_SOURCES += lib/yasmx/Leb128Bytecode.cpp
ygas_SOURCES += lib/yasmx/ListFormat.cpp
ygas_SOURCES += lib/yasmx/Location.cpp
ygas_SOURCES += lib/yasmx/Location_util.cpp
ygas_SOURCES += lib/yasmx/Module.cpp
ygas_SOURCES += lib/yasmx/MultipleBytecode.cpp
ygas_SOURCES += lib/yasmx/NumericOutput.cpp
ygas_SOURCES += lib/yasmx/Object.cpp
ygas_SOURCES += lib/yasmx/Object_optimize.cpp
ygas_SOURCES += lib/yasmx/ObjectFormat.cpp
ygas_SOURCES += lib/yasmx/Object_util.cpp
ygas_SOURCES += lib/yasmx/OrgBytecode.cpp
ygas_SOURCES += lib/yasmx/Op.cpp
ygas_SOURCES += lib/yasmx/Reloc.cpp
ygas_SOURCES += lib/yasmx/Section.cpp
ygas_SOURCES += lib/yasmx/StringTable.cpp
ygas_SOURCES += lib/yasmx/Symbol.cpp
ygas_SOURCES += lib/yasmx/Symbol_util.cpp
ygas_SOURCES += lib/yasmx/Value.cpp

ygas_SOURCES += modules/init_plugin.cpp

EXTRA_DIST += lib/llvm/System/Unix/Memory.inc
EXTRA_DIST += lib/llvm/System/Unix/Path.inc
EXTRA_DIST += lib/llvm/System/Unix/Process.inc
EXTRA_DIST += lib/llvm/System/Unix/TimeValue.inc
EXTRA_DIST += lib/llvm/System/Unix/Unix.h

# arch/x86
ygas_SOURCES += modules/arch/x86/X86Arch.cpp
ygas_SOURCES += modules/arch/x86/X86Arch.h
ygas_SOURCES += modules/arch/x86/X86Common.cpp
ygas_SOURCES += modules/arch/x86/X86Common.h
ygas_SOURCES += modules/arch/x86/X86EffAddr.cpp
ygas_SOURCES += modules/arch/x86/X86EffAddr.h
ygas_SOURCES += modules/arch/x86/X86General.cpp
ygas_SOURCES += modules/arch/x86/X86General.h
ygas_SOURCES += modules/arch/x86/X86Insn.cpp
ygas_SOURCES += modules/arch/x86/X86Insn.h
ygas_SOURCES += modules/arch/x86/X86Jmp.cpp
ygas_SOURCES += modules/arch/x86/X86Jmp.h
ygas_SOURCES += modules/arch/x86/X86JmpFar.cpp
ygas_SOURCES += modules/arch/x86/X86JmpFar.h
ygas_SOURCES += modules/arch/x86/X86Opcode.cpp
ygas_SOURCES += modules/arch/x86/X86Opcode.h
ygas_SOURCES += modules/arch/x86/X86Prefix.h
ygas_SOURCES += modules/arch/x86/X86Register.h
ygas_SOURCES += modules/arch/x86/X86RegisterGroup.h
ygas_SOURCES += modules/arch/x86/X86TargetModifier.h
nodist_ygas_SOURCES += X86Arch_cpu.cpp
nodist_ygas_SOURCES += X86Register.cpp
BUILT_SOURCES += X86Arch_cpu.cpp
BUILT_SOURCES += X86Register.cpp

$(srcdir)/modules/arch/x86/X86Insn.cpp: X86Insn_nasm.cpp X86Insn_gas.cpp X86Insns.cpp
X86Insn_nasm.gperf X86Insn_gas.gperf X86Insns.cpp: $(srcdir)/modules/arch/x86/gen_x86_insn.py
	$(PYTHON) $(srcdir)/modules/arch/x86/gen_x86_insn.py \
	    X86Insns.cpp X86Insn_gas.gperf X86Insn_nasm.gperf
BUILT_SOURCES += X86Insns.cpp
BUILT_SOURCES += X86Insn_gas.gperf
BUILT_SOURCES += X86Insn_nasm.gperf
EXTRA_DIST += modules/arch/x86/gen_x86_insn.py

X86Arch_cpu.cpp: $(srcdir)/modules/arch/x86/X86Arch_cpu.gperf genperf$(EXEEXT)
	$(top_builddir)/genperf$(EXEEXT) $(srcdir)/modules/arch/x86/X86Arch_cpu.gperf $@
EXTRA_DIST += modules/arch/x86/X86Arch_cpu.gperf

X86Register.cpp: $(srcdir)/modules/arch/x86/X86Register.gperf genperf$(EXEEXT)
	$(top_builddir)/genperf$(EXEEXT) $(srcdir)/modules/arch/x86/X86Register.gperf $@
EXTRA_DIST += modules/arch/x86/X86Register.gperf

X86Insn_nasm.cpp: X86Insn_nasm.gperf genperf$(EXEEXT)
	$(top_builddir)/genperf$(EXEEXT) X86Insn_nasm.gperf $@
BUILT_SOURCES += X86Insn_nasm.cpp
CLEANFILES += X86Insn_nasm.cpp

X86Insn_gas.cpp: X86Insn_gas.gperf genperf$(EXEEXT)
	$(top_builddir)/genperf$(EXEEXT) X86Insn_gas.gperf $@
BUILT_SOURCES += X86Insn_gas.cpp
CLEANFILES += X86Insn_gas.cpp

# dbgfmts/null
ygas_SOURCES += modules/dbgfmts/null/NullDebug.cpp
ygas_SOURCES += modules/dbgfmts/null/NullDebug.h

# dbgfmts/dwarf2
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Debug.cpp
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Debug.h
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Debug_aranges.cpp
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Debug_info.cpp
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Debug_line.cpp
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Section.cpp
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Section.h
ygas_SOURCES += modules/dbgfmts/dwarf2/Dwarf2Types.h

# objfmts/bin
ygas_SOURCES += modules/objfmts/bin/BinLink.cpp
ygas_SOURCES += modules/objfmts/bin/BinLink.h
ygas_SOURCES += modules/objfmts/bin/BinMapOutput.cpp
ygas_SOURCES += modules/objfmts/bin/BinMapOutput.h
ygas_SOURCES += modules/objfmts/bin/BinObject.cpp
ygas_SOURCES += modules/objfmts/bin/BinObject.h
ygas_SOURCES += modules/objfmts/bin/BinSection.cpp
ygas_SOURCES += modules/objfmts/bin/BinSection.h
ygas_SOURCES += modules/objfmts/bin/BinSymbol.cpp
ygas_SOURCES += modules/objfmts/bin/BinSymbol.h

# objfmts/elf
ygas_SOURCES += modules/objfmts/elf/ElfConfig.cpp
ygas_SOURCES += modules/objfmts/elf/ElfConfig.h
ygas_SOURCES += modules/objfmts/elf/ElfMachine.cpp
ygas_SOURCES += modules/objfmts/elf/ElfMachine.h
ygas_SOURCES += modules/objfmts/elf/ElfObject.cpp
ygas_SOURCES += modules/objfmts/elf/ElfObject.h
ygas_SOURCES += modules/objfmts/elf/ElfReloc.cpp
ygas_SOURCES += modules/objfmts/elf/ElfReloc.h
ygas_SOURCES += modules/objfmts/elf/ElfSection.cpp
ygas_SOURCES += modules/objfmts/elf/ElfSection.h
ygas_SOURCES += modules/objfmts/elf/ElfSymbol.cpp
ygas_SOURCES += modules/objfmts/elf/ElfSymbol.h
ygas_SOURCES += modules/objfmts/elf/ElfTypes.h
ygas_SOURCES += modules/objfmts/elf/Elf_x86_x86.cpp
ygas_SOURCES += modules/objfmts/elf/Elf_x86_amd64.cpp

# parsers/gas
ygas_SOURCES += modules/parsers/gas/GasLexer.cpp
ygas_SOURCES += modules/parsers/gas/GasLexer.h
ygas_SOURCES += modules/parsers/gas/GasNumericParser.cpp
ygas_SOURCES += modules/parsers/gas/GasNumericParser.h
ygas_SOURCES += modules/parsers/gas/GasParser.cpp
ygas_SOURCES += modules/parsers/gas/GasParser.h
ygas_SOURCES += modules/parsers/gas/GasParser_parse.cpp
ygas_SOURCES += modules/parsers/gas/GasPreproc.cpp
ygas_SOURCES += modules/parsers/gas/GasPreproc.h
ygas_SOURCES += modules/parsers/gas/GasStringParser.cpp
ygas_SOURCES += modules/parsers/gas/GasStringParser.h

# tools/genperf
noinst_PROGRAMS += genperf

genperf_SOURCES =
EXTRA_DIST += tools/genperf/genperf.cpp
EXTRA_DIST += tools/genperf/perfect.cpp
EXTRA_DIST += tools/genperf/perfect.h
EXTRA_DIST += tools/genperf/standard.h
genperf_LDADD  = genperf.$(OBJEXT)
genperf_LDADD += gp-perfect.$(OBJEXT)
genperf_LDADD += gp-phash.$(OBJEXT)
genperf_LINK = $(CXXLD_FOR_BUILD) -o $@

genperf.$(OBJEXT): $(srcdir)/tools/genperf/genperf.cpp
	$(CXX_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  -c -o $@ `test -f tools/genperf/genperf.cpp || echo '$(srcdir)/'`tools/genperf/genperf.cpp

gp-perfect.$(OBJEXT): $(srcdir)/tools/genperf/perfect.cpp
	$(CXX_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  -c -o $@ `test -f tools/genperf/perfect.cpp || echo '$(srcdir)/'`tools/genperf/perfect.cpp

gp-phash.$(OBJEXT): $(srcdir)/lib/yasmx/Support/phash.cpp
	$(CXX_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  -c -o $@ `test -f lib/yasmx/Support/phash.cpp || echo '$(srcdir)/'`lib/yasmx/Support/phash.cpp

# files needed for distribution
EXTRA_DIST += Copying/BSD.txt
EXTRA_DIST += Copying/COPYING
EXTRA_DIST += Copying/LLVM.txt
EXTRA_DIST += m4/func_mmap_file.m4
EXTRA_DIST += m4/header_mmap_anonymous.m4
EXTRA_DIST += m4/need_dev_zero_for_mmap.m4

DISTCLEANFILES  = $(BUILT_SOURCES)
DISTCLEANFILES += functional_aliases
DISTCLEANFILES += functional_includes
