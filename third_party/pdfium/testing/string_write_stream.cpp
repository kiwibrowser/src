// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/string_write_stream.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/widestring.h"

StringWriteStream::StringWriteStream() = default;

StringWriteStream::~StringWriteStream() = default;

FX_FILESIZE StringWriteStream::GetSize() {
  return stream_.tellp();
}

bool StringWriteStream::Flush() {
  return true;
}

bool StringWriteStream::WriteBlock(const void* pData,
                                   FX_FILESIZE offset,
                                   size_t size) {
  ASSERT(offset == 0);
  stream_.write(static_cast<const char*>(pData), size);
  return true;
}

bool StringWriteStream::WriteString(const ByteStringView& str) {
  stream_.write(str.unterminated_c_str(), str.GetLength());
  return true;
}
