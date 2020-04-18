// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_pagearea.h"

#include "xfa/fxfa/parser/cxfa_pagearea.h"

CJX_PageArea::CJX_PageArea(CXFA_PageArea* node) : CJX_Container(node) {}

CJX_PageArea::~CJX_PageArea() = default;

void CJX_PageArea::use(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::pagePosition(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::oddOrEven(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::relevant(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::initialNumber(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::usehref(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::numbered(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PageArea::blankOrNotBlank(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
