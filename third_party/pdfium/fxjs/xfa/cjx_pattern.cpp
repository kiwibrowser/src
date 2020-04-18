// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_pattern.h"

#include "xfa/fxfa/parser/cxfa_pattern.h"

CJX_Pattern::CJX_Pattern(CXFA_Pattern* node) : CJX_Node(node) {}

CJX_Pattern::~CJX_Pattern() = default;

void CJX_Pattern::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Pattern::type(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Pattern::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
