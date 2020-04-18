// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_decimal.h"

#include "xfa/fxfa/parser/cxfa_decimal.h"

CJX_Decimal::CJX_Decimal(CXFA_Decimal* node) : CJX_Content(node) {}

CJX_Decimal::~CJX_Decimal() = default;

void CJX_Decimal::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Decimal::fracDigits(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Decimal::defaultValue(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Som_DefaultValue(pValue, bSetting, eAttribute);
}

void CJX_Decimal::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Decimal::value(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Som_DefaultValue(pValue, bSetting, eAttribute);
}

void CJX_Decimal::leadDigits(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
