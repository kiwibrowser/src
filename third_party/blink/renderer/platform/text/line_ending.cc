/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/platform/text/line_ending.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/string_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

class OutputBuffer {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(OutputBuffer);

 public:
  OutputBuffer() = default;
  virtual char* Allocate(size_t) = 0;
  virtual void Copy(const CString&) = 0;
  virtual ~OutputBuffer() = default;
};

class CStringBuffer final : public OutputBuffer {
 public:
  CStringBuffer(CString& buffer) : buffer_(buffer) {}
  ~CStringBuffer() override = default;

  char* Allocate(size_t size) override {
    char* ptr;
    buffer_ = CString::CreateUninitialized(size, ptr);
    return ptr;
  }

  void Copy(const CString& source) override { buffer_ = source; }

  const CString& Buffer() const { return buffer_; }

 private:
  CString buffer_;
};

class VectorCharAppendBuffer final : public OutputBuffer {
 public:
  VectorCharAppendBuffer(Vector<char>& buffer) : buffer_(buffer) {}
  ~VectorCharAppendBuffer() override = default;

  char* Allocate(size_t size) override {
    size_t old_size = buffer_.size();
    buffer_.Grow(old_size + size);
    return buffer_.data() + old_size;
  }

  void Copy(const CString& source) override {
    buffer_.Append(source.data(), source.length());
  }

 private:
  Vector<char>& buffer_;
};

template <typename CharType>
size_t RequiredSizeForCRLF(const CharType* data, size_t length) {
  size_t new_len = 0;
  const CharType* p = data;
  while (p < data + length) {
    CharType c = *p++;
    if (c == '\r') {
      if (p >= data + length || *p != '\n') {
        // Turn CR into CRLF.
        new_len += 2;
      } else {
        // We already have \r\n. We don't count this \r, and the
        // following \n will count 2.
      }
    } else if (c == '\n') {
      // Turn LF into CRLF.
      new_len += 2;
    } else {
      // Leave other characters alone.
      new_len += 1;
    }
  }
  return new_len;
}

template <typename CharType>
void NormalizeToCRLF(const CharType* src, size_t src_length, CharType* q) {
  const CharType* p = src;
  while (p < src + src_length) {
    CharType c = *p++;
    if (c == '\r') {
      if (p >= src + src_length || *p != '\n') {
        // Turn CR into CRLF.
        *q++ = '\r';
        *q++ = '\n';
      }
    } else if (c == '\n') {
      // Turn LF into CRLF.
      *q++ = '\r';
      *q++ = '\n';
    } else {
      // Leave other characters alone.
      *q++ = c;
    }
  }
}

void InternalNormalizeLineEndingsToCRLF(const CString& from,
                                        OutputBuffer& buffer) {
  size_t new_len = RequiredSizeForCRLF(from.data(), from.length());
  if (new_len < from.length())
    return;

  if (new_len == from.length()) {
    buffer.Copy(from);
    return;
  }

  NormalizeToCRLF(from.data(), from.length(), buffer.Allocate(new_len));
}

}  // namespace

void NormalizeLineEndingsToLF(const CString& from, Vector<char>& result) {
  // Compute the new length.
  size_t new_len = 0;
  bool need_fix = false;
  const char* p = from.data();
  char from_ending_char = '\r';
  char to_ending_char = '\n';
  while (p < from.data() + from.length()) {
    char c = *p++;
    if (c == '\r' && *p == '\n') {
      // Turn CRLF into CR or LF.
      p++;
      need_fix = true;
    } else if (c == from_ending_char) {
      // Turn CR/LF into LF/CR.
      need_fix = true;
    }
    new_len += 1;
  }

  // Grow the result buffer.
  p = from.data();
  size_t old_result_size = result.size();
  result.Grow(old_result_size + new_len);
  char* q = result.data() + old_result_size;

  // If no need to fix the string, just copy the string over.
  if (!need_fix) {
    memcpy(q, p, from.length());
    return;
  }

  // Make a copy of the string.
  while (p < from.data() + from.length()) {
    char c = *p++;
    if (c == '\r' && *p == '\n') {
      // Turn CRLF or CR into CR or LF.
      p++;
      *q++ = to_ending_char;
    } else if (c == from_ending_char) {
      // Turn CR/LF into LF/CR.
      *q++ = to_ending_char;
    } else {
      // Leave other characters alone.
      *q++ = c;
    }
  }
}

CString NormalizeLineEndingsToCRLF(const CString& from) {
  if (!from.length())
    return from;
  CString result;
  CStringBuffer buffer(result);
  InternalNormalizeLineEndingsToCRLF(from, buffer);
  return buffer.Buffer();
}

String NormalizeLineEndingsToCRLF(const String& src) {
  size_t length = src.length();
  if (length == 0)
    return src;
  if (src.Is8Bit()) {
    size_t new_length = RequiredSizeForCRLF(src.Characters8(), length);
    if (new_length == length)
      return src;
    StringBuffer<LChar> buffer(new_length);
    NormalizeToCRLF(src.Characters8(), length, buffer.Characters());
    return String::Adopt(buffer);
  }
  size_t new_length = RequiredSizeForCRLF(src.Characters16(), length);
  if (new_length == length)
    return src;
  StringBuffer<UChar> buffer(new_length);
  NormalizeToCRLF(src.Characters16(), length, buffer.Characters());
  return String::Adopt(buffer);
}

void NormalizeLineEndingsToNative(const CString& from, Vector<char>& result) {
#if defined(OS_WIN)
  VectorCharAppendBuffer buffer(result);
  InternalNormalizeLineEndingsToCRLF(from, buffer);
#else
  NormalizeLineEndingsToLF(from, result);
#endif
}

}  // namespace blink
