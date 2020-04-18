// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfpf_skiabufferfont.h"

CFPF_SkiaBufferFont::CFPF_SkiaBufferFont()
    : m_pBuffer(nullptr), m_szBuffer(0) {}

CFPF_SkiaBufferFont::~CFPF_SkiaBufferFont() = default;

int32_t CFPF_SkiaBufferFont::GetType() const {
  return FPF_SKIAFONTTYPE_Buffer;
}
