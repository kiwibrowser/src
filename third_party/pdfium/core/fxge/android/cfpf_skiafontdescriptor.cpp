// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiafontdescriptor.h"

#include "core/fxcrt/fx_memory.h"

CFPF_SkiaFontDescriptor::CFPF_SkiaFontDescriptor()
    : m_pFamily(nullptr),
      m_dwStyle(0),
      m_iFaceIndex(0),
      m_dwCharsets(0),
      m_iGlyphNum(0) {}

CFPF_SkiaFontDescriptor::~CFPF_SkiaFontDescriptor() {
  FX_Free(m_pFamily);
}

int32_t CFPF_SkiaFontDescriptor::GetType() const {
  return FPF_SKIAFONTTYPE_Unknown;
}

void CFPF_SkiaFontDescriptor::SetFamily(const char* pFamily) {
  FX_Free(m_pFamily);
  int32_t iSize = strlen(pFamily);
  m_pFamily = FX_Alloc(char, iSize + 1);
  memcpy(m_pFamily, pFamily, iSize * sizeof(char));
  m_pFamily[iSize] = 0;
}
