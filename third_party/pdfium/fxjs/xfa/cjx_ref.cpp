// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_ref.h"

#include "xfa/fxfa/parser/cxfa_ref.h"

CJX_Ref::CJX_Ref(CXFA_Ref* node) : CJX_TextNode(node) {}

CJX_Ref::~CJX_Ref() = default;

void CJX_Ref::use(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Ref::usehref(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
