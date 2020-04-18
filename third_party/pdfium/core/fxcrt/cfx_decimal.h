// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_CFX_DECIMAL_H_
#define CORE_FXCRT_CFX_DECIMAL_H_

#include "core/fxcrt/fx_string.h"

class CFX_Decimal {
 public:
  CFX_Decimal();
  explicit CFX_Decimal(uint32_t val);
  explicit CFX_Decimal(uint64_t val);
  explicit CFX_Decimal(int32_t val);
  CFX_Decimal(float val, uint8_t scale);
  explicit CFX_Decimal(const WideStringView& str);

  operator WideString() const;
  operator double() const;

  CFX_Decimal operator*(const CFX_Decimal& val) const;
  CFX_Decimal operator/(const CFX_Decimal& val) const;

  void SetScale(uint8_t newScale);
  uint8_t GetScale();
  void SetNegate();

 private:
  CFX_Decimal(uint32_t hi, uint32_t mid, uint32_t lo, bool neg, uint8_t scale);
  bool IsNotZero() const { return m_uHi || m_uMid || m_uLo; }
  void Swap(CFX_Decimal& val);

  uint32_t m_uHi;
  uint32_t m_uLo;
  uint32_t m_uMid;
  uint32_t m_uFlags;
};

#endif  // CORE_FXCRT_CFX_DECIMAL_H_
