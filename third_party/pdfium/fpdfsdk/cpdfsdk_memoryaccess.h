// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_MEMORYACCESS_H_
#define FPDFSDK_CPDFSDK_MEMORYACCESS_H_

#include "core/fxcrt/fx_stream.h"

class CPDFSDK_MemoryAccess final : public IFX_SeekableReadStream {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_SeekableReadStream:
  FX_FILESIZE GetSize() override;
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override;

 private:
  CPDFSDK_MemoryAccess(const uint8_t* pBuf, FX_FILESIZE size);
  ~CPDFSDK_MemoryAccess() override;

  const uint8_t* const m_pBuf;
  const FX_FILESIZE m_size;
};

#endif  // FPDFSDK_CPDFSDK_MEMORYACCESS_H_
