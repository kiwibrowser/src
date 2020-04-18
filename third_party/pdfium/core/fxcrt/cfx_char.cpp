// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/cfx_char.h"

CFX_Char::CFX_Char(uint16_t wCharCode, uint32_t dwCharProps)
    : CFX_Char(wCharCode, dwCharProps, 100, 100) {}

CFX_Char::CFX_Char(uint16_t wCharCode,
                   uint32_t dwCharProps,
                   int32_t iHorizontalScale,
                   int32_t iVerticalScale)
    : m_dwStatus(CFX_BreakType::None),
      m_nBreakType(0),
      m_dwCharStyles(0),
      m_iCharWidth(0),
      m_iBidiClass(0),
      m_iBidiLevel(0),
      m_iBidiPos(0),
      m_iBidiOrder(0),
      m_iFontSize(0),
      m_dwIdentity(0),
      m_wCharCode(wCharCode),
      m_dwCharProps(dwCharProps),
      m_iHorizontalScale(iHorizontalScale),
      m_iVerticalScale(iVerticalScale) {}

CFX_Char::CFX_Char(const CFX_Char& other) = default;

CFX_Char::~CFX_Char() = default;

FX_CHARTYPE CFX_Char::GetCharType() const {
  return GetCharTypeFromProp(m_dwCharProps);
}
