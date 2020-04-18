/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_QUOTED_PRINTABLE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_QUOTED_PRINTABLE_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Delegate for controling the behavior of quoted-printable encoding. The
// original characters may be encoded a bit differently depending on where
// they live, header or body. For example, "=CRLF" should be used to break
// long line in body while "CRLF+SPACE" should be used to break long line in
// header.
class PLATFORM_EXPORT QuotedPrintableEncodeDelegate {
 public:
  QuotedPrintableEncodeDelegate() = default;
  virtual ~QuotedPrintableEncodeDelegate() = default;

  // Returns maximum number of characters allowed for an encoded line, excluding
  // prefix and soft line break.
  virtual size_t GetMaxLineLengthForEncodedContent() const = 0;

  // Returns true if space and tab characters need to be encoded.
  virtual bool ShouldEncodeWhiteSpaceCharacters(bool end_of_line) const = 0;

  // Called when an encoded line starts. The delegate can take this chance to
  // add any prefix.
  virtual void DidStartLine(Vector<char>& out) = 0;

  // Called when an encoded line ends. The delegate can take this chance to add
  // any suffix. If it is not last line, a soft line break should also
  // be added after the suffix.
  virtual void DidFinishLine(bool last_line, Vector<char>& out) = 0;
};

PLATFORM_EXPORT void QuotedPrintableEncode(const char*,
                                           size_t,
                                           QuotedPrintableEncodeDelegate*,
                                           Vector<char>&);

PLATFORM_EXPORT void QuotedPrintableDecode(const char*, size_t, Vector<char>&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_QUOTED_PRINTABLE_H_
