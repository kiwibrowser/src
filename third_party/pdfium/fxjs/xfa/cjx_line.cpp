// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_line.h"

#include "xfa/fxfa/parser/cxfa_line.h"

CJX_Line::CJX_Line(CXFA_Line* node) : CJX_Node(node) {}

CJX_Line::~CJX_Line() = default;

void CJX_Line::use(CFXJSE_Value* pValue,
                   bool bSetting,
                   XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Line::slope(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Line::usehref(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Line::hand(CFXJSE_Value* pValue,
                    bool bSetting,
                    XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
