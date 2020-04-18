// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_rootelement.h"

#include "xfa/fxfa/parser/cxfa_rootelement.h"

CJX_RootElement::CJX_RootElement(CXFA_RootElement* node) : CJX_TextNode(node) {}

CJX_RootElement::~CJX_RootElement() = default;

void CJX_RootElement::use(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_RootElement::usehref(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
