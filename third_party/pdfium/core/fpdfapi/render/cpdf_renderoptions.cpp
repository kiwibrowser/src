// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_renderoptions.h"

CPDF_RenderOptions::CPDF_RenderOptions()
    : m_ColorMode(kNormal),
      m_Flags(RENDER_CLEARTYPE),
      m_dwLimitCacheSize(1024 * 1024 * 100),
      m_bDrawAnnots(false) {}

CPDF_RenderOptions::CPDF_RenderOptions(const CPDF_RenderOptions& rhs)
    : m_ColorMode(rhs.m_ColorMode),
      m_Flags(rhs.m_Flags),
      m_dwLimitCacheSize(rhs.m_dwLimitCacheSize),
      m_bDrawAnnots(rhs.m_bDrawAnnots),
      m_pOCContext(rhs.m_pOCContext) {}

CPDF_RenderOptions::~CPDF_RenderOptions() {}

FX_ARGB CPDF_RenderOptions::TranslateColor(FX_ARGB argb) const {
  if (ColorModeIs(kNormal))
    return argb;
  if (ColorModeIs(kAlpha))
    return argb;

  int a;
  int r;
  int g;
  int b;
  std::tie(a, r, g, b) = ArgbDecode(argb);
  int gray = FXRGB2GRAY(r, g, b);
  return ArgbEncode(a, gray, gray, gray);
}
