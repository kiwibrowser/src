/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Torch Mobile, Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ios/third_party/blink/src/html_tokenizer.h"

#include "html_markup_tokenizer_inlines.h"

namespace WebCore {

#define HTML_BEGIN_STATE(stateName) BEGIN_STATE(HTMLTokenizer, stateName)
#define HTML_RECONSUME_IN(stateName) RECONSUME_IN(HTMLTokenizer, stateName)
#define HTML_ADVANCE_TO(stateName) ADVANCE_TO(HTMLTokenizer, stateName)
#define HTML_SWITCH_TO(stateName) SWITCH_TO(HTMLTokenizer, stateName)

HTMLTokenizer::HTMLTokenizer()
    : m_state(HTMLTokenizer::DataState)
    , m_token(nullptr)
    , m_additionalAllowedCharacter('\0')
    , m_inputStreamPreprocessor(this)
{
}

HTMLTokenizer::~HTMLTokenizer()
{
}

void HTMLTokenizer::reset()
{
    m_state = HTMLTokenizer::DataState;
    m_token = 0;
    m_additionalAllowedCharacter = '\0';
}

bool HTMLTokenizer::flushBufferedEndTag(CharacterProvider& source)
{
    ASSERT(m_token->type() == HTMLToken::Character || m_token->type() == HTMLToken::Uninitialized);
    source.next();
    if (m_token->type() == HTMLToken::Character)
        return true;

    return false;
}

#define FLUSH_AND_ADVANCE_TO(stateName)                                    \
    do {                                                                   \
        m_state = HTMLTokenizer::stateName;                           \
        if (flushBufferedEndTag(source))                                   \
            return true;                                                   \
        if (source.isEmpty()                                               \
            || !m_inputStreamPreprocessor.peek(source))                    \
            return haveBufferedCharacterToken();                           \
        cc = m_inputStreamPreprocessor.nextInputCharacter();               \
        goto stateName;                                                    \
    } while (false)

bool HTMLTokenizer::nextToken(CharacterProvider& source, HTMLToken& token)
{
    // If we have a token in progress, then we're supposed to be called back
    // with the same token so we can finish it.
    ASSERT(!m_token || m_token == &token || token.type() == HTMLToken::Uninitialized);
    m_token = &token;

    if (source.isEmpty() || !m_inputStreamPreprocessor.peek(source))
        return haveBufferedCharacterToken();
    UChar cc = m_inputStreamPreprocessor.nextInputCharacter();

    // Source: http://www.whatwg.org/specs/web-apps/current-work/#tokenisation0
    switch (m_state) {
    HTML_BEGIN_STATE(DataState) {
        if (cc == '<') {
            if (m_token->type() == HTMLToken::Character) {
                // We have a bunch of character tokens queued up that we
                // are emitting lazily here.
                return true;
            }
            HTML_ADVANCE_TO(TagOpenState);
        } else if (cc == kEndOfFileMarker)
            return emitEndOfFile(source);
        else {
            m_token->ensureIsCharacterToken();
            HTML_ADVANCE_TO(DataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(TagOpenState) {
        if (cc == '!')
            HTML_ADVANCE_TO(MarkupDeclarationOpenState);
        else if (cc == '/')
            HTML_ADVANCE_TO(EndTagOpenState);
        else if (isASCIIUpper(cc)) {
            m_token->beginStartTag(toLowerCase(cc));
            HTML_ADVANCE_TO(TagNameState);
        } else if (isASCIILower(cc)) {
            m_token->beginStartTag(cc);
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == '?') {
            parseError();
            // The spec consumes the current character before switching
            // to the bogus comment state, but it's easier to implement
            // if we reconsume the current character.
            HTML_RECONSUME_IN(BogusCommentState);
        } else {
            parseError();
            m_token->ensureIsCharacterToken();
            HTML_RECONSUME_IN(DataState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(EndTagOpenState) {
        if (isASCIIUpper(cc)) {
            m_token->beginEndTag(static_cast<LChar>(toLowerCase(cc)));
            HTML_ADVANCE_TO(TagNameState);
        } else if (isASCIILower(cc)) {
            m_token->beginEndTag(static_cast<LChar>(cc));
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == '>') {
            parseError();
            HTML_ADVANCE_TO(DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->ensureIsCharacterToken();
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BogusCommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(TagNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            m_token->appendToName(toLowerCase(cc));
            HTML_ADVANCE_TO(TagNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            m_token->appendToName(cc);
            HTML_ADVANCE_TO(TagNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeAttributeNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=')
                parseError();
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeNameState) {
        if (isTokenizerWhitespace(cc)) {
            HTML_ADVANCE_TO(AfterAttributeNameState);
        } else if (cc == '/') {
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        } else if (cc == '=') {
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        } else if (cc == '>') {
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (isASCIIUpper(cc)) {
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=')
                parseError();
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterAttributeNameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '=')
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (isASCIIUpper(cc)) {
            HTML_ADVANCE_TO(AttributeNameState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<')
                parseError();
            HTML_ADVANCE_TO(AttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeAttributeValueState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeValueState);
        else if (cc == '"') {
            HTML_ADVANCE_TO(AttributeValueDoubleQuotedState);
        } else if (cc == '&') {
            HTML_RECONSUME_IN(AttributeValueUnquotedState);
        } else if (cc == '\'') {
            HTML_ADVANCE_TO(AttributeValueSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '<' || cc == '=' || cc == '`')
                parseError();
            HTML_ADVANCE_TO(AttributeValueUnquotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueDoubleQuotedState) {
        if (cc == '"') {
            HTML_ADVANCE_TO(AfterAttributeValueQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            HTML_ADVANCE_TO(AttributeValueDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueSingleQuotedState) {
        if (cc == '\'') {
            HTML_ADVANCE_TO(AfterAttributeValueQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            HTML_ADVANCE_TO(AttributeValueSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AttributeValueUnquotedState) {
        if (isTokenizerWhitespace(cc)) {
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        } else if (cc == '>') {
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            if (cc == '"' || cc == '\'' || cc == '<' || cc == '=' || cc == '`')
                parseError();
            HTML_ADVANCE_TO(AttributeValueUnquotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterAttributeValueQuotedState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeAttributeNameState);
        else if (cc == '/')
            HTML_ADVANCE_TO(SelfClosingStartTagState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeAttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(SelfClosingStartTagState) {
        if (cc == '>') {
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            HTML_RECONSUME_IN(DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeAttributeNameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BogusCommentState) {
        m_token->beginComment();
        HTML_RECONSUME_IN(ContinueBogusCommentState);
    }
    END_STATE()

    HTML_BEGIN_STATE(ContinueBogusCommentState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker)
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        else {
            HTML_ADVANCE_TO(ContinueBogusCommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(MarkupDeclarationOpenState) {
        DEFINE_STATIC_LOCAL_STRING(dashDashString, "--");
        DEFINE_STATIC_LOCAL_STRING(doctypeString, "doctype");
        if (cc == '-') {
            if (source.startsWith(dashDashString, dashDashStringLength)) {
                advanceAndASSERT(source, '-');
                advanceAndASSERT(source, '-');
                m_token->beginComment();
                HTML_SWITCH_TO(CommentStartState);
            } else if (source.remainingBytes() < dashDashStringLength)
                return haveBufferedCharacterToken();
        } else if (cc == 'D' || cc == 'd') {
            if (source.startsWith(doctypeString, doctypeStringLength, true)) {
                advanceStringAndASSERTIgnoringCase(source, doctypeString);
                HTML_SWITCH_TO(DOCTYPEState);
            } else if (source.remainingBytes() < doctypeStringLength)
                return haveBufferedCharacterToken();
        }
        parseError();
        HTML_RECONSUME_IN(BogusCommentState);
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentStartState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentStartDashState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentStartDashState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndDashState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndDashState) {
        if (cc == '-')
            HTML_ADVANCE_TO(CommentEndState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '!') {
            parseError();
            HTML_ADVANCE_TO(CommentEndBangState);
        } else if (cc == '-') {
            parseError();
            HTML_ADVANCE_TO(CommentEndState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CommentEndBangState) {
        if (cc == '-') {
            HTML_ADVANCE_TO(CommentEndDashState);
        } else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(CommentState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPENameState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->beginDOCTYPE();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_RECONSUME_IN(BeforeDOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPENameState);
        else if (cc == '>') {
            parseError();
            m_token->beginDOCTYPE();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            m_token->beginDOCTYPE();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            m_token->beginDOCTYPE();
            HTML_ADVANCE_TO(DOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPENameState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(DOCTYPENameState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPENameState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPENameState);
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            DEFINE_STATIC_LOCAL_STRING(publicString, "public");
            DEFINE_STATIC_LOCAL_STRING(systemString, "system");
            if (cc == 'P' || cc == 'p') {
                if (source.startsWith(publicString, publicStringLength, true)) {
                    advanceStringAndASSERTIgnoringCase(source, publicString);
                    HTML_SWITCH_TO(AfterDOCTYPEPublicKeywordState);
                } else if (source.remainingBytes() < publicStringLength)
                    return haveBufferedCharacterToken();
            } else if (cc == 'S' || cc == 's') {
                if (source.startsWith(systemString, systemStringLength, true)) {
                    advanceStringAndASSERTIgnoringCase(source, systemString);
                    HTML_SWITCH_TO(AfterDOCTYPESystemKeywordState);
                } else if (source.remainingBytes() < systemStringLength)
                    return haveBufferedCharacterToken();
            }
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPEPublicKeywordState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPEPublicIdentifierState);
        else if (cc == '"') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPEPublicIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPEPublicIdentifierState);
        else if (cc == '"') {
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEPublicIdentifierDoubleQuotedState) {
        if (cc == '"')
            HTML_ADVANCE_TO(AfterDOCTYPEPublicIdentifierState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPEPublicIdentifierSingleQuotedState) {
        if (cc == '\'')
            HTML_ADVANCE_TO(AfterDOCTYPEPublicIdentifierState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(DOCTYPEPublicIdentifierSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPEPublicIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BetweenDOCTYPEPublicAndSystemIdentifiersState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '"') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BetweenDOCTYPEPublicAndSystemIdentifiersState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BetweenDOCTYPEPublicAndSystemIdentifiersState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == '"') {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPESystemKeywordState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPESystemIdentifierState);
        else if (cc == '"') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            parseError();
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BeforeDOCTYPESystemIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(BeforeDOCTYPESystemIdentifierState);
        if (cc == '"') {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        } else if (cc == '\'') {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        } else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPESystemIdentifierDoubleQuotedState) {
        if (cc == '"')
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierDoubleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(DOCTYPESystemIdentifierSingleQuotedState) {
        if (cc == '\'')
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>') {
            parseError();
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        } else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            HTML_ADVANCE_TO(DOCTYPESystemIdentifierSingleQuotedState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(AfterDOCTYPESystemIdentifierState) {
        if (isTokenizerWhitespace(cc))
            HTML_ADVANCE_TO(AfterDOCTYPESystemIdentifierState);
        else if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker) {
            parseError();
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        } else {
            parseError();
            HTML_ADVANCE_TO(BogusDOCTYPEState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(BogusDOCTYPEState) {
        if (cc == '>')
            return emitAndResumeIn(source, HTMLTokenizer::DataState);
        else if (cc == kEndOfFileMarker)
            return emitAndReconsumeIn(source, HTMLTokenizer::DataState);
        HTML_ADVANCE_TO(BogusDOCTYPEState);
    }
    END_STATE()

    HTML_BEGIN_STATE(CDATASectionState) {
        if (cc == ']')
            HTML_ADVANCE_TO(CDATASectionRightSquareBracketState);
        else if (cc == kEndOfFileMarker)
            HTML_RECONSUME_IN(DataState);
        else {
            m_token->ensureIsCharacterToken();
            HTML_ADVANCE_TO(CDATASectionState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CDATASectionRightSquareBracketState) {
        if (cc == ']')
            HTML_ADVANCE_TO(CDATASectionDoubleRightSquareBracketState);
        else {
            m_token->ensureIsCharacterToken();
            HTML_RECONSUME_IN(CDATASectionState);
        }
    }
    END_STATE()

    HTML_BEGIN_STATE(CDATASectionDoubleRightSquareBracketState) {
        if (cc == '>')
            HTML_ADVANCE_TO(DataState);
        else {
            m_token->ensureIsCharacterToken();
            HTML_RECONSUME_IN(CDATASectionState);
        }
    }
    END_STATE()

    }

    ASSERT_NOT_REACHED();
    return false;
}

inline void HTMLTokenizer::parseError()
{
    notImplemented();
}

}
