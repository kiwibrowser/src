// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_memoryaccess.h"

CPDFSDK_MemoryAccess::CPDFSDK_MemoryAccess(const uint8_t* pBuf,
                                           FX_FILESIZE size)
    : m_pBuf(pBuf), m_size(size) {}

CPDFSDK_MemoryAccess::~CPDFSDK_MemoryAccess() = default;

FX_FILESIZE CPDFSDK_MemoryAccess::GetSize() {
  return m_size;
}

bool CPDFSDK_MemoryAccess::ReadBlock(void* buffer,
                                     FX_FILESIZE offset,
                                     size_t size) {
  if (offset < 0)
    return false;

  FX_SAFE_FILESIZE newPos = pdfium::base::checked_cast<FX_FILESIZE>(size);
  newPos += offset;
  if (!newPos.IsValid() || newPos.ValueOrDie() > m_size)
    return false;

  memcpy(buffer, m_pBuf + offset, size);
  return true;
}
