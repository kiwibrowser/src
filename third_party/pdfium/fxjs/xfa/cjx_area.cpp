// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_area.h"

#include "xfa/fxfa/parser/cxfa_area.h"

CJX_Area::CJX_Area(CXFA_Area* node) : CJX_Container(node) {}

CJX_Area::~CJX_Area() = default;

void CJX_Area::colSpan(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Area::relevant(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Area::use(CFXJSE_Value* pValue,
                   bool bSetting,
                   XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Area::usehref(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Area::x(CFXJSE_Value* pValue,
                 bool bSetting,
                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Area::y(CFXJSE_Value* pValue,
                 bool bSetting,
                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
