/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SUBSTITUTE_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SUBSTITUTE_DATA_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class SubstituteData {
  DISALLOW_NEW();

 public:
  SubstituteData() = default;

  SubstituteData(scoped_refptr<SharedBuffer> content)
      : SubstituteData(content, "text/html", "UTF-8", KURL()) {}

  SubstituteData(scoped_refptr<SharedBuffer> content,
                 const AtomicString& mime_type,
                 const AtomicString& text_encoding,
                 const KURL& failing_url)
      : content_(std::move(content)),
        mime_type_(mime_type),
        text_encoding_(text_encoding),
        failing_url_(failing_url) {}

  bool IsValid() const { return content_.get(); }

  SharedBuffer* Content() const { return content_.get(); }
  const AtomicString& MimeType() const { return mime_type_; }
  const AtomicString& TextEncoding() const { return text_encoding_; }
  const KURL& FailingURL() const { return failing_url_; }

 private:
  scoped_refptr<SharedBuffer> content_;
  AtomicString mime_type_;
  AtomicString text_encoding_;
  KURL failing_url_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_SUBSTITUTE_DATA_H_
