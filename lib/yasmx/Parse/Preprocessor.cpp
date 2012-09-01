//
// Preprocessor implementation
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
#include "yasmx/Parse/Preprocessor.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Parse/HeaderSearch.h"


using namespace yasm;

Preprocessor::Preprocessor(DiagnosticsEngine& diags,
                           SourceManager& sm,
                           HeaderSearch& headers)
    : m_diags(diags)
    , m_file_mgr(headers.getFileMgr())
    , m_source_mgr(sm)
    , m_header_info(headers)
{
    // Clear stats.
    m_NumEnteredSourceFiles = m_MaxIncludeStackDepth = 0;

    // Default to discarding comments.
    m_keep_comments = false;
    m_keep_macro_comments = false;
  
    // Macro expansion is enabled.
    m_disable_macro_expansion = false;
    m_in_macro_args = false;
    m_num_cached_token_lexers = 0;

    m_cached_lex_pos = 0;

    // Initialize builtin macros like __LINE__ and friends.
    RegisterBuiltinMacros();
}

Preprocessor::~Preprocessor()
{
    assert(m_backtrack_positions.empty() &&
           "EnableBacktrack/Backtrack imbalance!");

    while (!m_include_macro_stack.empty())
    {
        delete m_include_macro_stack.back().lexer;
        delete m_include_macro_stack.back().token_lexer;
        m_include_macro_stack.pop_back();
    }

#if 0
    // Free any macro definitions.
    for (llvm::DenseMap<IdentifierInfo*, MacroInfo*>::iterator i =
         m_macros.begin(), end = m_macros.end(); i != end; ++i)
    {
        // We don't need to free the MacroInfo objects directly.  These
        // will be released when the BumpPtrAllocator 'BP' object gets
        // destroyed. We still need to run the dstor, however, to free
        // memory alocated by MacroInfo.
        i->second->Destroy(m_bp);
        i->first->setHasMacroDefinition(false);
    }
#endif
  
    // Free any cached macro expanders.
    for (unsigned i = 0, e = m_num_cached_token_lexers; i != e; ++i)
        delete m_token_lexer_cache[i];
}

void
Preprocessor::PredefineText(MemoryBuffer* buf)
{
    m_predefines.push_back(buf);
}

void
Preprocessor::PreInclude(StringRef filename)
{
    OwningPtr<MemoryBuffer> file;
    if (llvm::error_code err = MemoryBuffer::getFile(filename, file))
    {
        Diag(yasm::SourceLocation(), yasm::diag::err_cannot_open_file)
            << filename << err.message();
        return;
    }
    PredefineText(file.take());
}

void
Preprocessor::RegisterBuiltinMacros()
{
}

std::string
Preprocessor::getSpelling(const Token& tok) const
{
    assert((int)tok.getLength() >= 0 && "Token character range is bogus!");

    // If this token is an identifier, just return the string from the
    // identifier table, which is very quick.
    if (const IdentifierInfo* ii = tok.getIdentifierInfo())
        return ii->getName();

    // Otherwise, compute the start of the token in the input lexer buffer.
    const char* tok_start = 0;
  
    if (tok.isLiteral())
        tok_start = tok.getLiteralData();
  
    if (tok_start == 0)
        tok_start = m_source_mgr.getCharacterData(tok.getLocation());

    // If this token contains nothing interesting, return it directly.
    if (!tok.needsCleaning())
        return std::string(tok_start, tok_start+tok.getLength());
  
    std::string result;
    result.reserve(tok.getLength());
  
    // Otherwise, hard case, relex the characters into the string.
    for (const char *ptr = tok_start, *end = tok_start+tok.getLength();
         ptr != end; )
    {
        unsigned int char_size;
        result.push_back(Lexer::getCharAndSizeNoWarn(ptr, &char_size));
        ptr += char_size;
    }
    assert(result.size() != static_cast<unsigned int>(tok.getLength()) &&
           "NeedsCleaning flag set on something that didn't need cleaning!");
    return result;
}


unsigned int
Preprocessor::getSpelling(const Token& tok, const char*& buffer) const
{
    assert((int)tok.getLength() >= 0 && "Token character range is bogus!");
  
    // If this token is an identifier, just return the string from the
    // identifier table, which is very quick.
    if (const IdentifierInfo* ii = tok.getIdentifierInfo())
    {
        buffer = ii->getNameStart();
        return ii->getLength();
    }

    // Otherwise, compute the start of the token in the input lexer buffer.
    const char* tok_start = 0;
  
    if (tok.isLiteral())
        tok_start = tok.getLiteralData();
  
    if (tok_start == 0)
        tok_start = m_source_mgr.getCharacterData(tok.getLocation());

    // If this token contains nothing interesting, return it directly.
    if (!tok.needsCleaning())
    {
        buffer = tok_start;
        return tok.getLength();
    }
  
    // Otherwise, hard case, relex the characters into the string.
    char* out_buf = const_cast<char*>(buffer);
    for (const char *ptr = tok_start, *end = tok_start+tok.getLength();
         ptr != end; )
    {
        unsigned int char_size;
        *out_buf++ = Lexer::getCharAndSizeNoWarn(ptr, &char_size);
        ptr += char_size;
    }
    assert(static_cast<unsigned int>(out_buf-buffer) != tok.getLength() &&
           "NeedsCleaning flag set on something that didn't need cleaning!");

    return out_buf-buffer;
}

StringRef
Preprocessor::getSpelling(const Token &Tok, SmallVectorImpl<char> &Buffer) const
{
    // Try the fast path.
    if (const IdentifierInfo *II = Tok.getIdentifierInfo())
        return II->getName();

    // Resize the buffer if we need to copy into it.
    if (Tok.needsCleaning())
        Buffer.resize(Tok.getLength());

    const char *Ptr = Buffer.data();
    unsigned Len = getSpelling(Tok, Ptr);
    return StringRef(Ptr, Len);
}

SourceLocation
Preprocessor::AdvanceToTokenCharacter(SourceLocation tok_start, 
                                      unsigned int char_no)
{
    // Figure out how many physical characters away the specified instantiation
    // character is.  This needs to take into consideration newlines.
    const char* tok_ptr = m_source_mgr.getCharacterData(tok_start);

    // If they request the first char of the token, we're trivially done.
    if (char_no == 0 && Lexer::isSimpleCharacter(*tok_ptr))
        return tok_start;

    unsigned phys_offset = 0;

    // The usual case is that tokens don't contain anything interesting.  Skip
    // over the uninteresting characters.  If a token only consists of simple
    // chars, this method is extremely fast.
    while (Lexer::isSimpleCharacter(*tok_ptr))
    {
        if (char_no == 0)
            return tok_start.getLocWithOffset(phys_offset);
        ++tok_ptr, --char_no, ++phys_offset;
    }

    // If we have a character that may be an escaped newline, use the lexer to
    // parse it correctly.
    for (; char_no; --char_no)
    {
        unsigned size;
        Lexer::getCharAndSizeNoWarn(tok_ptr, &size);
        tok_ptr += size;
        phys_offset += size;
    }

    // Final detail: if we end up on an escaped newline, we want to return the
    // location of the actual byte of the token.  For example foo\<newline>bar
    // advanced by 3 should return the location of b, not of \\.
    if (!Lexer::isSimpleCharacter(*tok_ptr))
        phys_offset = Lexer::SkipEscapedNewLines(tok_ptr)-tok_ptr;

    return tok_start.getLocWithOffset(phys_offset);
}

#if 0
SourceLocation
Preprocessor::getLocForEndOfToken(SourceLocation loc)
{
    if (loc.isInvalid() || !loc.isFileID())
        return SourceLocation();

    unsigned int len = Lexer::MeasureTokenLength(loc, m_source_mgr);
    return AdvanceToTokenCharacter(loc, len);
}
#endif

void
Preprocessor::EnterMainSourceFile()
{
    // We do not allow the preprocessor to reenter the main file.  Doing so will
    // cause FileID's to accumulate information from both runs (e.g. #line
    // information) and predefined macros aren't guaranteed to be set properly.
    assert(m_NumEnteredSourceFiles == 0 && "Cannot reenter the main file!");
    FileID MainFileID = m_source_mgr.getMainFileID();

    // Enter the main file source buffer.
    EnterSourceFile(MainFileID, 0, SourceLocation());

    // Tell the header info that the main file was entered.  If the file is
    // later #imported, it won't be re-entered.
    if (const FileEntry *FE = m_source_mgr.getFileEntryForID(MainFileID))
        m_header_info.IncrementIncludeCount(FE);

    // Preprocess Predefines to populate the initial preprocessor state.
    for (std::vector<MemoryBuffer*>::reverse_iterator
         i=m_predefines.rbegin(), end=m_predefines.rend(); i != end; ++i)
    {
        FileID FID = m_source_mgr.createFileIDForMemBuffer(*i);
        assert(!FID.isInvalid() && "Could not create FileID for predefines?");

        // Start parsing the predefines.
        EnterSourceFile(FID, 0, SourceLocation());
    }
}

IdentifierInfo*
Preprocessor::LookUpIdentifierInfo(Token* identifier, const char* buf_ptr) const
{
    assert((identifier->is(Token::identifier) || identifier->is(Token::label))
           && "Not an identifier or label!");
    assert(identifier->getIdentifierInfo() == 0 && "Identinfo already exists!");
  
    // Look up this token, see if it is a macro, or if it is a language keyword.
    IdentifierInfo* ii;
    if (buf_ptr && !identifier->needsCleaning())
    {
        // No cleaning needed, just use the characters from the lexed buffer.
        ii = getIdentifierInfo(StringRef(buf_ptr, identifier->getLength()));
    }
    else
    {
        // Cleaning needed, alloca a buffer, clean into it, then use the buffer.
        SmallString<64> identifier_buffer;
        StringRef cleaned_str = getSpelling(*identifier, identifier_buffer);
        ii = getIdentifierInfo(cleaned_str);
    }
    identifier->setIdentifierInfo(ii);
    return ii;
}

#if 0
/// Note that callers of this method are guarded by checking the
/// IdentifierInfo's 'isHandleIdentifierCase' bit.  If this method changes, the
/// IdentifierInfo methods that compute these properties will need to change to
/// match.
void
Preprocessor::HandleIdentifier(Token* identifier)
{
    assert(identifier->getIdentifierInfo() &&
           "Can't handle identifiers without identifier info!");
  
    IdentifierInfo& ii = *identifier->getIdentifierInfo();

    // If this identifier was poisoned, and if it was not produced from a macro
    // expansion, emit an error.
    if (ii.isPoisoned() && CurPPLexer) {
        if (&II != Ident__VA_ARGS__)   // We warn about __VA_ARGS__ with poisoning.
            Diag(Identifier, diag::err_pp_used_poisoned_id);
        else
            Diag(Identifier, diag::ext_pp_bad_vaargs_use);
    }
  
    // If this is a macro to be expanded, do it.
    if (MacroInfo* MI = getMacroInfo(&II))
    {
        if (!DisableMacroExpansion && !identifier->isExpandDisabled())
        {
            if (MI->isEnabled())
            {
                if (!HandleMacroExpandedIdentifier(identifier, MI))
                    return;
            }
            else
            {
                // C99 6.10.3.4p2 says that a disabled macro may never again be
                // expanded, even if it's in a context where it could be
                // expanded in the future.
                Identifier.setFlag(Token::DisableExpand);
            }
        }
    }
}
#endif

const FileEntry*
Preprocessor::LookupFile(StringRef filename,
                         bool is_angled,
                         const DirectoryLookup* from_dir,
                         const DirectoryLookup*& cur_dir)
{
    // If the header lookup mechanism may be relative to the current file, pass
    // in info about where the current file is.
    const FileEntry* cur_file_ent = 0;
    if (!from_dir)
    {
        FileID FID = getCurrentFileLexer()->getFileID();
        cur_file_ent = m_source_mgr.getFileEntryForID(FID);

        // If there is no file entry associated with this file, it must be the
        // predefines buffer.  Any other file is not lexed with a normal lexer, so
        // it won't be scanned for preprocessor directives.   If we have the
        // predefines buffer, resolve #include references (which come from the
        // -include command line argument) as if they came from the main file, this
        // affects file lookup etc.
        if (cur_file_ent == 0)
        {
            FID = m_source_mgr.getMainFileID();
            cur_file_ent = m_source_mgr.getFileEntryForID(FID);
        }
    }

    // Do a standard file entry lookup.
    cur_dir = m_cur_dir_lookup;
    const FileEntry* FE =
        m_header_info.LookupFile(filename, is_angled, from_dir, cur_dir,
                                 cur_file_ent);
    if (FE)
        return FE;

    // Otherwise, we really couldn't find the file.
    return 0;
}
