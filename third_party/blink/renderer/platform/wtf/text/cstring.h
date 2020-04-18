/*
 * Copyright (C) 2003, 2006, 2008, 2009, 2010, 2012 Apple Inc. All rights
 * reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CSTRING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CSTRING_H_

#include <string.h>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partition_allocator.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/wtf_export.h"

namespace WTF {

// CStringImpl is an immutable ref-counted storage for the characters in a
// CString. It's analogous to a StringImpl but may contain any arbitrary
// sequence of bytes. The data is always allocated 1 longer than length() and is
// null terminated.
class WTF_EXPORT CStringImpl : public RefCounted<CStringImpl> {
 public:
  // CStringImpls are allocated out of the WTF buffer partition.
  void* operator new(size_t, void* ptr) { return ptr; }
  void operator delete(void*);

  static scoped_refptr<CStringImpl> CreateUninitialized(size_t length,
                                                        char*& data);

  const char* data() const { return reinterpret_cast<const char*>(this + 1); }
  size_t length() const { return length_; }

 private:
  explicit CStringImpl(size_t length) : length_(length) {}

  const unsigned length_;

  DISALLOW_COPY_AND_ASSIGN(CStringImpl);
};

// A container for an immutable ref-counted null-terminated char array. This is
// analogous to a WTF::String but does not require the contained bytes to be
// valid Latin1 or UTF-16. Instead a CString can contain any arbitrary bytes.
class WTF_EXPORT CString {
  USING_FAST_MALLOC(CString);

 public:
  // Construct a null string, distinguishable from an empty string.
  CString() = default;

  // Construct a string from arbitrary bytes.
  CString(const char* chars) : CString(chars, chars ? strlen(chars) : 0) {}
  CString(const char*, size_t length);

  // Construct a string referencing an existing buffer.
  CString(CStringImpl* buffer) : buffer_(buffer) {}
  CString(scoped_refptr<CStringImpl> buffer) : buffer_(std::move(buffer)) {}

  static CString CreateUninitialized(size_t length, char*& data) {
    return CStringImpl::CreateUninitialized(length, data);
  }

  // The bytes of the string, always NUL terminated. May be null.
  const char* data() const { return buffer_ ? buffer_->data() : nullptr; }

  // The length of the data(), *not* including the NUL terminator.
  size_t length() const { return buffer_ ? buffer_->length() : 0; }

  bool IsNull() const { return !buffer_; }

  bool IsSafeToSendToAnotherThread() const;

  CStringImpl* Impl() const { return buffer_.get(); }

 private:
  scoped_refptr<CStringImpl> buffer_;
};

WTF_EXPORT bool operator==(const CString& a, const CString& b);
inline bool operator!=(const CString& a, const CString& b) {
  return !(a == b);
}
WTF_EXPORT bool operator==(const CString& a, const char* b);
inline bool operator!=(const CString& a, const char* b) {
  return !(a == b);
}

// Pretty printer for gtest and base/logging.*.  It prepends and appends
// double-quotes, and escapes characters other than ASCII printables.
WTF_EXPORT std::ostream& operator<<(std::ostream&, const CString&);

}  // namespace WTF

using WTF::CString;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CSTRING_H_
