/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/xml/parser/xml_document_parser_scope.h"

namespace blink {

Document* XMLDocumentParserScope::current_document_ = nullptr;

XMLDocumentParserScope::XMLDocumentParserScope(Document* document)
    : old_document_(current_document_),
      old_generic_error_func_(xmlGenericError),
      old_structured_error_func_(xmlStructuredError),
      old_error_context_(xmlGenericErrorContext) {
  current_document_ = document;
}

XMLDocumentParserScope::XMLDocumentParserScope(
    Document* document,
    xmlGenericErrorFunc generic_error_func,
    xmlStructuredErrorFunc structured_error_func,
    void* error_context)
    : old_document_(current_document_),
      old_generic_error_func_(xmlGenericError),
      old_structured_error_func_(xmlStructuredError),
      old_error_context_(xmlGenericErrorContext) {
  current_document_ = document;
  if (generic_error_func)
    xmlSetGenericErrorFunc(error_context, generic_error_func);
  if (structured_error_func)
    xmlSetStructuredErrorFunc(error_context, structured_error_func);
}

XMLDocumentParserScope::~XMLDocumentParserScope() {
  current_document_ = old_document_;
  xmlSetGenericErrorFunc(old_error_context_, old_generic_error_func_);
  xmlSetStructuredErrorFunc(old_error_context_, old_structured_error_func_);
}

}  // namespace blink
