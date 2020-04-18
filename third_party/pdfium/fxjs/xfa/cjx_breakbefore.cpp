// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_breakbefore.h"

#include "xfa/fxfa/parser/cxfa_breakbefore.h"

CJX_BreakBefore::CJX_BreakBefore(CXFA_BreakBefore* node) : CJX_Node(node) {}

CJX_BreakBefore::~CJX_BreakBefore() = default;

void CJX_BreakBefore::use(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::startNew(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::trailer(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::targetType(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::usehref(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::target(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_BreakBefore::leader(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
