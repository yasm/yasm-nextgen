#ifndef YASM_PARSE_PREPROCESSOR_H
#define YASM_PARSE_PREPROCESSOR_H
//
// Preprocessor header file
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2009  Peter Johnson
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
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/IdentifierTable.h"
#include "yasmx/Parse/Lexer.h"
#include "yasmx/Parse/Token.h"
#include "yasmx/Parse/TokenLexer.h"


namespace yasm
{

class DirectoryLookup;
class HeaderSearch;

class YASM_LIB_EXPORT Preprocessor
{
public:
    Preprocessor(Diagnostic& diags, SourceManager& sm, HeaderSearch& headers);
    virtual ~Preprocessor();

    Diagnostic& getDiagnostics() const { return m_diags; }
    SourceManager& getSourceManager() const { return m_source_mgr; }
    HeaderSearch& getHeaderSearch() const { return m_header_info; }
    IdentifierTable& getIdentifierTable() { return m_identifiers; }
    llvm::BumpPtrAllocator& getPreprocessorAllocator() { return m_bp; }

    /// Control whether or not the preprocessor retains comments in output.
    void setCommentRetentionState(bool keep_comments, bool keep_macro_comments)
    {
        m_keep_comments = keep_comments | keep_macro_comments;
        m_keep_macro_comments = keep_macro_comments;
    }
  
    bool getCommentRetentionState() const { return m_keep_comments; }
  
    /// Return true if we are lexing directly from the specified lexer.
    bool isCurrentLexer(const Lexer* l) const { return m_cur_lexer.get() == l; }

    /// Return the current file lexer being lexed from.  Note
    /// that this ignores any potentially active macro expansions and _Pragma
    /// expansions going on at the time.
    Lexer* getCurrentFileLexer() const;

    /// Return information about the specified preprocessor
    /// identifier token.
    IdentifierInfo* getIdentifierInfo(llvm::StringRef name) const
    {
        return &m_identifiers.get(name);
    }

    /// Pre-include arbitrary text content (e.g. from a file).
    /// @param buf      memory buffer
    virtual void PredefineText(llvm::MemoryBuffer* buf);

    /// Pre-include file.
    /// @param filename filename
    virtual void PreInclude(llvm::StringRef filename);

    /// Pre-define a macro.
    /// @param macronameval "name=value" string
    virtual void PredefineMacro(llvm::StringRef macronameval) = 0;

    /// Un-define a macro.
    /// @param macroname    macro name
    virtual void UndefineMacro(llvm::StringRef macroname) = 0;

    /// Define a builtin macro, preprocessed before the "standard" macros.
    /// @param macronameval "name=value" string
    virtual void DefineBuiltin(llvm::StringRef macronameval) = 0;

    /// Enter the specified FileID as the main source file,
    /// which implicitly adds the builtin defines etc.
    void EnterMainSourceFile();
  
    /// Add a source file to the top of the include stack and
    /// start lexing tokens from it instead of the current buffer.  Return true
    /// and fill in ErrorStr with the error information on failure.
    void EnterSourceFile(FileID cur_file_id,
                         const DirectoryLookup* dir,
                         SourceLocation loc);

    /// Add a "macro" context to the top of the include stack,
    /// which will cause the lexer to start returning the specified tokens.
    ///
    /// If DisableMacroExpansion is true, tokens lexed from the token stream will
    /// not be subject to further macro expansion.  Otherwise, these tokens will
    /// be re-macro-expanded when/if expansion is enabled.
    ///
    /// If OwnsTokens is false, this method assumes that the specified stream of
    /// tokens has a permanent owner somewhere, so they do not need to be copied.
    /// If it is true, it assumes the array of tokens is allocated with new[] and
    /// must be freed.
    void EnterTokenStream(const Token* toks,
                          unsigned int num_toks,
                          bool disable_macro_expansion,
                          bool owns_tokens);

    /// Pop the current lexer/macro exp off the top of the
    /// lexer stack.  This should only be used in situations where the current
    /// state of the top-of-stack lexer is known.
    void RemoveTopOfLexerStack();

    /// From the point that this method is called, and until
    /// CommitBacktrackedTokens() or Backtrack() is called, the Preprocessor
    /// keeps track of the lexed tokens so that a subsequent Backtrack() call
    /// will make the Preprocessor re-lex the same tokens.
    ///
    /// Nested backtracks are allowed, meaning that
    /// EnableBacktrackAtThisPos can be called multiple times and
    /// CommitBacktrackedTokens/Backtrack calls will be combined with the
    /// EnableBacktrackAtThisPos calls in reverse order.
    ///
    /// NOTE: *DO NOT* forget to call either CommitBacktrackedTokens or
    /// Backtrack at some point after EnableBacktrackAtThisPos. If you
    /// don't, caching of tokens will continue indefinitely.
    void EnableBacktrackAtThisPos();

    /// Disable the last EnableBacktrackAtThisPos() call.
    void CommitBacktrackedTokens();

    /// Make Preprocessor re-lex the tokens that were lexed since
    /// EnableBacktrackAtThisPos() was previously called. 
    void Backtrack();

    /// Return true if EnableBacktrackAtThisPos() was called and
    /// caching of tokens is on.
    bool isBacktrackEnabled() const { return !m_backtrack_positions.empty(); }

    /// To lex a token from the preprocessor, just pull a token from the
    /// current lexer or macro object.
    void Lex(Token* result)
    {
        if (m_cur_lexer.get() != 0)
            m_cur_lexer->Lex(result);
        else if (m_cur_token_lexer.get() != 0)
            m_cur_token_lexer->Lex(result);
        else
            CachingLex(result);
    }
  
    /// This peeks ahead N tokens and returns that token without consuming
    /// any tokens.  LookAhead(0) returns the next token that would be
    /// returned by Lex(), LookAhead(1) returns the token after it, etc.
    const Token& LookAhead(unsigned int n=0)
    {
        if (m_cached_lex_pos + n < m_cached_tokens.size())
            return m_cached_tokens[m_cached_lex_pos+n];
        else
            return PeekAhead(n+1);
    }

    /// When backtracking is enabled and tokens are cached,
    /// this allows to revert a specific number of tokens.
    /// Note that the number of tokens being reverted should be up to the last
    /// backtrack position, not more.
    void RevertCachedTokens(unsigned int n)
    {
        assert(isBacktrackEnabled() &&
               "Should only be called when tokens are cached for backtracking");
        assert(signed(m_cached_lex_pos) - signed(n)
               >= signed(m_backtrack_positions.back())
            && "Should revert tokens up to the last backtrack position, not more");
        assert(signed(m_cached_lex_pos) - signed(n) >= 0 &&
               "Corrupted backtrack positions ?");
        m_cached_lex_pos -= n;
    }

    /// Enters a token in the token stream to be lexed next. If backtrack()
    /// is called afterwards, the token will remain at the insertion point.
    void EnterToken(const Token& tok)
    {
        EnterCachingLexMode();
        m_cached_tokens.insert(m_cached_tokens.begin()+m_cached_lex_pos, tok);
    }

    /// Forwarding function for diagnostics.  This emits a diagnostic at
    /// the specified Token's location, translating the token's start
    /// position in the current buffer into a SourcePosition object for
    /// rendering.
    DiagnosticBuilder Diag(SourceLocation loc, unsigned int id)
    {
        return m_diags.Report(loc, id);
    }

    DiagnosticBuilder Diag(const Token& tok, unsigned int id)
    {
        return m_diags.Report(tok.getLocation(), id);
    }
  
    /// Return the 'spelling' of the Tok token.  The spelling of a
    /// token is the characters used to represent the token in the source file
    /// after escaped-newline folding.  In particular, this wants to get the
    /// true, uncanonicalized, spelling.
    std::string getSpelling(const Token& tok) const;

    /// This method is used to get the spelling of a token into a preallocated
    /// buffer, instead of as an std::string.  The caller is required to
    /// allocate enough space for the token, which is guaranteed to be at least
    /// tok.getLength() bytes long.  The length of the actual result is
    /// returned.
    ///
    /// Note that this method may do two possible things: it may either fill in
    /// the buffer specified with characters, or it may *change the input
    /// pointer* to point to a constant buffer with the data already in it
    /// (avoiding a copy).  The caller is not allowed to modify the returned
    /// buffer pointer if an internal buffer is returned.
    unsigned int getSpelling(const Token& tok, const char*& buffer) const;

    /// getSpelling - This method is used to get the spelling of a token into a
    /// SmallVector. Note that the returned StringRef may not point to the
    /// supplied buffer if a copy can be avoided.
    llvm::StringRef getSpelling(const Token& tok,
                                llvm::SmallVectorImpl<char>& buffer) const;

    /// \brief Computes the source location just past the end of the
    /// token at this source location.
    ///
    /// This routine can be used to produce a source location that
    /// points just past the end of the token referenced by \p Loc, and
    /// is generally used when a diagnostic needs to point just after a
    /// token where it expected something different that it received. If
    /// the returned source location would not be meaningful (e.g., if
    /// it points into a macro), this routine returns an invalid
    /// source location.
    SourceLocation getLocForEndOfToken(SourceLocation Loc);

    /// Given a location that specifies the start of a
    /// token, return a new location that specifies a character within the token.
    SourceLocation AdvanceToTokenCharacter
        (SourceLocation tok_start, unsigned int char_no);


    // Preprocessor callback methods.  These are invoked by a lexer as various
    // directives and events are found.

    /// Given a NasmToken::identifier token, look up the
    /// identifier information for the token and install it into the token.
    IdentifierInfo* LookUpIdentifierInfo(Token* identifier,
                                         const char* buf_ptr = 0) const;

    /// This callback is invoked when the lexer reads an identifier and has
    /// filled in the token's IdentifierInfo member.  This callback
    /// potentially macro expands it or turns it into a named token (like
    /// 'for').
    void HandleIdentifier(Token* identifier);

    /// This callback is invoked when the lexer hits the end of the current
    /// file.  This either returns the EOF token and returns true, or pops a
    /// level off the include stack and returns false, at which point the
    /// client should call lex again.
    bool HandleEndOfFile(Token* result, bool is_end_of_macro = false);

    /// HandleEndOfTokenLexer - This callback is invoked when the current
    /// TokenLexer hits the end of its token stream.
    bool HandleEndOfTokenLexer(Token* result);

    /// LookupFile - Given a "foo" or <foo> reference, look up the indicated file,
    /// return null on failure.  isAngled indicates whether the file reference is
    /// for system #include's or not (i.e. using <> instead of "").
    const FileEntry* LookupFile(llvm::StringRef filename,
                                bool is_angled,
                                const DirectoryLookup* from_dir,
                                const DirectoryLookup*& cur_dir);

    /// GetCurDirLookup - The DirectoryLookup structure used to find the current
    /// FileEntry, if CurLexer is non-null and if applicable.  This allows us to
    /// implement #include_next and find directory-specific properties.
    const DirectoryLookup* GetCurDirLookup() { return m_cur_dir_lookup; }

    /// isInPrimaryFile - Return true if we're in the top-level file, not in a
    /// #include.
    bool isInPrimaryFile() const;

protected:
    Diagnostic& m_diags;
    FileManager& m_file_mgr;
    SourceManager& m_source_mgr;
    HeaderSearch& m_header_info;

    /// A BumpPtrAllocator object used to quickly allocate and release
    /// objects internal to the preprocessor.
    llvm::BumpPtrAllocator m_bp;
    
    enum
    {
        /// Maximum depth of includes.
        MaxAllowedIncludeStackDepth = 200
    };

    // State that is set before the preprocessor begins.
    bool m_keep_comments : 1;
    bool m_keep_macro_comments : 1;

    // State that changes while the preprocessor runs:
    bool m_in_macro_args : 1; // True if parsing fn macro invocation args.

    /// True if macro expansion is disabled.
    bool m_disable_macro_expansion : 1;

    /// Mapping/lookup information for all identifiers in
    /// the program, including program keywords.
    mutable IdentifierTable m_identifiers;

    /// This is the current top of the stack that we're lexing from if
    /// not expanding a macro and we are lexing directly from source code.
    /// Only one of m_cur_lexer or m_cur_token_lexer will be non-null.
    llvm::OwningPtr<Lexer> m_cur_lexer;

    /// CurLookup - The DirectoryLookup structure used to find the current
    /// FileEntry, if CurLexer is non-null and if applicable.  This allows us to
    /// implement #include_next and find directory-specific properties.
    const DirectoryLookup* m_cur_dir_lookup;

    /// This is the current macro we are expanding, if we are expanding a
    /// macro.  One of m_cur_lexer and m_cur_token_lexer must be null.
    llvm::OwningPtr<TokenLexer> m_cur_token_lexer;

    /// This keeps track of the stack of files currently
    /// %included, and macros currently being expanded from, not counting
    /// m_cur_lexer/m_cur_token_lexer.
    struct IncludeStackInfo
    {
        Lexer*                  lexer;
        TokenLexer*             token_lexer;    
        const DirectoryLookup*  dir_lookup;

        IncludeStackInfo(Lexer* l, TokenLexer* tl, const DirectoryLookup *d)
            : lexer(l), token_lexer(tl), dir_lookup(d) {}
    };
    std::vector<IncludeStackInfo> m_include_macro_stack;

#if 0
    /// These are actions invoked when some preprocessor activity is
    /// encountered (e.g. a file is #included, etc).
    PPCallbacks* m_callbacks;
  
    /// For each IdentifierInfo with 'HasMacro' set, we keep a mapping
    /// to the actual definition of the macro.
    llvm::DenseMap<IdentifierInfo*, MacroInfo*> m_macros;

    /// A "freelist" of MacroInfo objects that can be reused for quick
    ///  allocation.
    std::vector<MacroInfo*> m_MI_cache;
#endif
    // Various statistics we track for performance analysis.
    unsigned int m_NumEnteredSourceFiles, m_MaxIncludeStackDepth;

    /// This string is the predefined macros that preprocessor
    /// should use from the command line etc.
    std::vector<llvm::MemoryBuffer*> m_predefines;
  
    /// Cache macro expanders to reduce malloc traffic.
    enum { TokenLexerCacheSize = 8 };
    unsigned int m_num_cached_token_lexers;
    TokenLexer* m_token_lexer_cache[TokenLexerCacheSize];

    // Cached tokens state.
    typedef llvm::SmallVector<Token, 1> CachedTokens;

    /// Cached tokens are stored here when we do backtracking or
    /// lookahead. They are "lexed" by the caching_lex() method.
    CachedTokens m_cached_tokens;

    /// CachedLexPos - The position of the cached token that CachingLex() should
    /// "lex" next. If it points beyond the CachedTokens vector, it means that
    /// a normal Lex() should be invoked.
    CachedTokens::size_type m_cached_lex_pos;

    /// BacktrackPositions - Stack of backtrack positions, allowing nested
    /// backtracks. The EnableBacktrackAtThisPos() method pushes a position to
    /// indicate where CachedLexPos should be set when the BackTrack() method is
    /// invoked (at which point the last position is popped).
    std::vector<CachedTokens::size_type> m_backtrack_positions;

    /// Register builtin macros such as __LINE__ with the identifier table.
    /// Default implementation does nothing.
    virtual void RegisterBuiltinMacros();

    /// Factory function to make a new lexer.
    virtual Lexer* CreateLexer(FileID fid,
                               const llvm::MemoryBuffer* input_buffer) = 0;

#if 0
    /// Factory function to make a new raw lexer.
    virtual Lexer* CreateRawLexer
        (SourceLocation file_loc,
         const char *buf_start,
         const char *buf_ptr,
         const char *buf_end) = 0;
#endif

private:
    Preprocessor(const Preprocessor&);      // DO NOT IMPLEMENT
    void operator=(const Preprocessor&);    // DO NOT IMPLEMENT

    void PushIncludeMacroStack()
    {
        m_include_macro_stack.push_back(
            IncludeStackInfo(m_cur_lexer.take(), m_cur_token_lexer.take(),
                             m_cur_dir_lookup));
    }

    void PopIncludeMacroStack()
    {
        m_cur_lexer.reset(m_include_macro_stack.back().lexer);
        m_cur_token_lexer.reset(m_include_macro_stack.back().token_lexer);
        m_cur_dir_lookup  = m_include_macro_stack.back().dir_lookup;
        m_include_macro_stack.pop_back();
    }

    /// EnterSourceFileWithLexer - Add a lexer to the top of the include stack and
    /// start lexing tokens from it instead of the current buffer.
    void EnterSourceFileWithLexer(Lexer* lexer, const DirectoryLookup* dir);

    /// Returns true if we are lexing from a file and not a macro.
    static bool IsFileLexer(const Lexer* lexer)
    {
        return lexer != 0;
    }
    static bool IsFileLexer(const IncludeStackInfo& info)
    {
        return IsFileLexer(info.lexer);
    }
    bool IsFileLexer() const
    {
        return IsFileLexer(m_cur_lexer.get());
    }

    // Caching stuff.
    void CachingLex(Token* result);
    bool isInCachingLexMode() const
    {
        return m_cur_lexer == 0 && m_cur_token_lexer == 0;
    }
    void EnterCachingLexMode();
    void ExitCachingLexMode()
    {
        if (isInCachingLexMode())
            RemoveTopOfLexerStack();
    }
    const Token& PeekAhead(unsigned int n);

    // File inclusion.
    void HandleIncludeDirective(Token& tok,
                                const DirectoryLookup* lookup_from = 0);
};

} // namespace yasm

#endif
