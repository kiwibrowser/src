// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_STRING_WRITE_STREAM_H_
#define TESTING_STRING_WRITE_STREAM_H_

#include <sstream>
#include <string>

#include "core/fxcrt/fx_stream.h"

class StringWriteStream : public IFX_SeekableWriteStream {
 public:
  StringWriteStream();
  ~StringWriteStream() override;

  // IFX_SeekableWriteStream
  FX_FILESIZE GetSize() override;
  bool Flush() override;
  bool WriteBlock(const void* pData, FX_FILESIZE offset, size_t size) override;
  bool WriteString(const ByteStringView& str) override;

  std::string ToString() const { return stream_.str(); }

 private:
  std::ostringstream stream_;
};

#endif  // TESTING_STRING_WRITE_STREAM_H_
