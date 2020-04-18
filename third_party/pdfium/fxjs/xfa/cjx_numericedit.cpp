// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_numericedit.h"

#include "xfa/fxfa/parser/cxfa_numericedit.h"

CJX_NumericEdit::CJX_NumericEdit(CXFA_NumericEdit* node) : CJX_Node(node) {}

CJX_NumericEdit::~CJX_NumericEdit() = default;

void CJX_NumericEdit::use(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_NumericEdit::usehref(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_NumericEdit::hScrollPolicy(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
