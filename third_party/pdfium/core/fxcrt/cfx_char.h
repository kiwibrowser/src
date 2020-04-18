// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_CFX_CHAR_H_
#define CORE_FXCRT_CFX_CHAR_H_

#include <stdint.h>

#include "core/fxcrt/fx_unicode.h"

enum class CFX_BreakType { None = 0, Piece, Line, Paragraph, Page };

class CFX_Char {
 public:
  CFX_Char(uint16_t wCharCode, uint32_t dwCharProps);
  CFX_Char(uint16_t wCharCode,
           uint32_t dwCharProps,
           int32_t iHorizontalScale,
           int32_t iVerticalScale);
  CFX_Char(const CFX_Char& other);
  ~CFX_Char();

  FX_CHARTYPE GetCharType() const;

  uint16_t char_code() const { return m_wCharCode; }
  uint32_t char_props() const { return m_dwCharProps; }
  int16_t horizonal_scale() const { return m_iHorizontalScale; }
  int16_t vertical_scale() const { return m_iVerticalScale; }

  CFX_BreakType m_dwStatus;
  uint8_t m_nBreakType;
  uint32_t m_dwCharStyles;
  int32_t m_iCharWidth;
  int16_t m_iBidiClass;
  uint16_t m_iBidiLevel;
  uint16_t m_iBidiPos;
  uint16_t m_iBidiOrder;
  int32_t m_iFontSize;
  uint32_t m_dwIdentity;
  RetainPtr<Retainable> m_pUserData;

 private:
  uint16_t m_wCharCode;
  uint32_t m_dwCharProps;
  int32_t m_iHorizontalScale;
  int32_t m_iVerticalScale;
};

#endif  // CORE_FXCRT_CFX_CHAR_H_
