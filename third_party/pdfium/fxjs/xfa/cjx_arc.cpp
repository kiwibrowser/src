// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_arc.h"

#include "xfa/fxfa/parser/cxfa_arc.h"

CJX_Arc::CJX_Arc(CXFA_Arc* node) : CJX_Node(node) {}

CJX_Arc::~CJX_Arc() = default;

void CJX_Arc::circular(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Arc::hand(CFXJSE_Value* pValue,
                   bool bSetting,
                   XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Arc::startAngle(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Arc::sweepAngle(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Arc::use(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Arc::usehref(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
