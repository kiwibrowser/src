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

#include "third_party/blink/renderer/platform/wtf/text/cstring.h"

#include <string.h>
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/ascii_ctype.h"

namespace WTF {

scoped_refptr<CStringImpl> CStringImpl::CreateUninitialized(size_t length,
                                                            char*& data) {
  // TODO(esprehn): This doesn't account for the NUL.
  CHECK_LT(length,
           (std::numeric_limits<unsigned>::max() - sizeof(CStringImpl)));

  // The +1 is for the terminating NUL character.
  size_t size = sizeof(CStringImpl) + length + 1;
  CStringImpl* buffer = static_cast<CStringImpl*>(
      Partitions::BufferMalloc(size, WTF_HEAP_PROFILER_TYPE_NAME(CStringImpl)));
  data = reinterpret_cast<char*>(buffer + 1);
  data[length] = '\0';
  return base::AdoptRef(new (buffer) CStringImpl(length));
}

void CStringImpl::operator delete(void* ptr) {
  Partitions::BufferFree(ptr);
}

CString::CString(const char* chars, size_t length) {
  if (!chars) {
    DCHECK_EQ(length, 0u);
    return;
  }
  char* data;
  buffer_ = CStringImpl::CreateUninitialized(length, data);
  memcpy(data, chars, length);
}

bool CString::IsSafeToSendToAnotherThread() const {
  return !buffer_ || buffer_->HasOneRef();
}

bool operator==(const CString& a, const CString& b) {
  if (a.IsNull() != b.IsNull())
    return false;
  if (a.length() != b.length())
    return false;
  return !memcmp(a.data(), b.data(), a.length());
}

bool operator==(const CString& a, const char* b) {
  if (a.IsNull() != !b)
    return false;
  if (!b)
    return true;
  return !strcmp(a.data(), b);
}

std::ostream& operator<<(std::ostream& ostream, const CString& string) {
  if (string.IsNull())
    return ostream << "<null>";

  ostream << '"';
  for (size_t index = 0; index < string.length(); ++index) {
    // Print shorthands for select cases.
    char character = string.data()[index];
    switch (character) {
      case '\t':
        ostream << "\\t";
        break;
      case '\n':
        ostream << "\\n";
        break;
      case '\r':
        ostream << "\\r";
        break;
      case '"':
        ostream << "\\\"";
        break;
      case '\\':
        ostream << "\\\\";
        break;
      default:
        if (IsASCIIPrintable(character)) {
          ostream << character;
        } else {
          // Print "\xHH" for control or non-ASCII characters.
          ostream << "\\x";
          if (character >= 0 && character < 0x10)
            ostream << "0";
          ostream.setf(std::ios_base::hex, std::ios_base::basefield);
          ostream.setf(std::ios::uppercase);
          ostream << (character & 0xff);
        }
        break;
    }
  }
  return ostream << '"';
}

}  // namespace WTF
