// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_customaccess.h"

CPDFSDK_CustomAccess::CPDFSDK_CustomAccess(FPDF_FILEACCESS* pFileAccess)
    : m_FileAccess(*pFileAccess) {}

CPDFSDK_CustomAccess::~CPDFSDK_CustomAccess() = default;

FX_FILESIZE CPDFSDK_CustomAccess::GetSize() {
  return m_FileAccess.m_FileLen;
}

bool CPDFSDK_CustomAccess::ReadBlock(void* buffer,
                                     FX_FILESIZE offset,
                                     size_t size) {
  if (offset < 0)
    return false;

  FX_SAFE_FILESIZE newPos = pdfium::base::checked_cast<FX_FILESIZE>(size);
  newPos += offset;
  if (!newPos.IsValid() ||
      newPos.ValueOrDie() > static_cast<FX_FILESIZE>(m_FileAccess.m_FileLen)) {
    return false;
  }
  return !!m_FileAccess.m_GetBlock(m_FileAccess.m_Param, offset,
                                   static_cast<uint8_t*>(buffer), size);
}
