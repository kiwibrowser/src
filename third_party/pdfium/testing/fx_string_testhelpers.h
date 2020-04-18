// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_FX_STRING_TESTHELPERS_H_
#define TESTING_FX_STRING_TESTHELPERS_H_

#include <ostream>

#include "core/fxcrt/cfx_datetime.h"
#include "core/fxcrt/fx_stream.h"

// Output stream operator so GTEST macros work with CFX_DateTime objects.
std::ostream& operator<<(std::ostream& os, const CFX_DateTime& dt);

class CFX_InvalidSeekableReadStream : public IFX_SeekableReadStream {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_SeekableReadStream overrides:
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override {
    return false;
  }
  FX_FILESIZE GetSize() override { return data_size_; }

 private:
  explicit CFX_InvalidSeekableReadStream(FX_FILESIZE data_size);
  ~CFX_InvalidSeekableReadStream() override;

  const FX_FILESIZE data_size_;
};

class CFX_BufferSeekableReadStream : public IFX_SeekableReadStream {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_SeekableReadStream:
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override {
    if (offset < 0 || static_cast<size_t>(offset) >= data_size_)
      return false;

    if (static_cast<size_t>(offset) + size > data_size_)
      size = data_size_ - static_cast<size_t>(offset);
    if (size == 0)
      return false;

    memcpy(buffer, data_ + offset, size);
    return true;
  }

  FX_FILESIZE GetSize() override {
    return static_cast<FX_FILESIZE>(data_size_);
  }

 private:
  CFX_BufferSeekableReadStream(const unsigned char* src, size_t src_size);
  ~CFX_BufferSeekableReadStream() override;

  const unsigned char* data_;
  size_t data_size_;
};

#endif  // TESTING_FX_STRING_TESTHELPERS_H_
