// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_ANDROID_CFPF_SKIAFONTDESCRIPTOR_H_
#define CORE_FXGE_ANDROID_CFPF_SKIAFONTDESCRIPTOR_H_

#include "core/fxcrt/fx_system.h"

#define FPF_SKIAFONTTYPE_Unknown 0

class CFPF_SkiaFontDescriptor {
 public:
  CFPF_SkiaFontDescriptor();
  virtual ~CFPF_SkiaFontDescriptor();

  virtual int32_t GetType() const;

  void SetFamily(const char* pFamily);

  char* m_pFamily;
  uint32_t m_dwStyle;
  int32_t m_iFaceIndex;
  uint32_t m_dwCharsets;
  int32_t m_iGlyphNum;
};

#endif  // CORE_FXGE_ANDROID_CFPF_SKIAFONTDESCRIPTOR_H_
