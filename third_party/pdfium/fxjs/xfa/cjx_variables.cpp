// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_variables.h"

#include "xfa/fxfa/parser/cxfa_variables.h"

CJX_Variables::CJX_Variables(CXFA_Variables* node) : CJX_Container(node) {}

CJX_Variables::~CJX_Variables() = default;

void CJX_Variables::use(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Variables::usehref(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
