// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_subformset.h"

#include "xfa/fxfa/parser/cxfa_subformset.h"

CJX_SubformSet::CJX_SubformSet(CXFA_SubformSet* node) : CJX_Container(node) {}

CJX_SubformSet::~CJX_SubformSet() = default;

void CJX_SubformSet::use(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SubformSet::relation(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SubformSet::relevant(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SubformSet::usehref(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SubformSet::instanceIndex(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Script_Som_InstanceIndex(pValue, bSetting, eAttribute);
}
