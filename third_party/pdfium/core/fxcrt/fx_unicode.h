// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_UNICODE_H_
#define CORE_FXCRT_FX_UNICODE_H_

#include "core/fxcrt/retain_ptr.h"

uint32_t FX_GetUnicodeProperties(wchar_t wch);
wchar_t FX_GetMirrorChar(wchar_t wch);

#ifdef PDF_ENABLE_XFA

// As defined in http://www.unicode.org/reports/tr14
constexpr uint8_t kBreakPropertySpace = 35;
constexpr uint8_t kBreakPropertyTB = 37;  // Don't know what this is ...

constexpr uint32_t FX_CHARTYPEBITS = 11;
constexpr uint32_t FX_CHARTYPEBITSMASK = 0xF << FX_CHARTYPEBITS;

enum FX_CHARTYPE {
  FX_CHARTYPE_Unknown = 0,
  FX_CHARTYPE_Tab = (1 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Space = (2 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Control = (3 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Combination = (4 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Numeric = (5 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Normal = (6 << FX_CHARTYPEBITS),
  FX_CHARTYPE_ArabicAlef = (7 << FX_CHARTYPEBITS),
  FX_CHARTYPE_ArabicSpecial = (8 << FX_CHARTYPEBITS),
  FX_CHARTYPE_ArabicDistortion = (9 << FX_CHARTYPEBITS),
  FX_CHARTYPE_ArabicNormal = (10 << FX_CHARTYPEBITS),
  FX_CHARTYPE_ArabicForm = (11 << FX_CHARTYPEBITS),
  FX_CHARTYPE_Arabic = (12 << FX_CHARTYPEBITS),
};

inline FX_CHARTYPE GetCharTypeFromProp(uint32_t prop) {
  return static_cast<FX_CHARTYPE>(prop & FX_CHARTYPEBITSMASK);
}

wchar_t FX_GetMirrorChar(wchar_t wch, uint32_t dwProps);

#endif  // PDF_ENABLE_XFA

#endif  // CORE_FXCRT_FX_UNICODE_H_
