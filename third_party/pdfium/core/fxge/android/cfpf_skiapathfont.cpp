// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiapathfont.h"

#include "core/fxcrt/fx_memory.h"

CFPF_SkiaPathFont::CFPF_SkiaPathFont() : m_pPath(nullptr) {}

CFPF_SkiaPathFont::~CFPF_SkiaPathFont() {
  FX_Free(m_pPath);
}

int32_t CFPF_SkiaPathFont::GetType() const {
  return FPF_SKIAFONTTYPE_Path;
}

void CFPF_SkiaPathFont::SetPath(const char* pPath) {
  FX_Free(m_pPath);
  int32_t iSize = strlen(pPath);
  m_pPath = FX_Alloc(char, iSize + 1);
  memcpy(m_pPath, pPath, iSize * sizeof(char));
  m_pPath[iSize] = 0;
}
