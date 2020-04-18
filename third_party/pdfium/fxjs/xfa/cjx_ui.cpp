// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_ui.h"

#include "xfa/fxfa/parser/cxfa_ui.h"

CJX_Ui::CJX_Ui(CXFA_Ui* node) : CJX_Node(node) {}

CJX_Ui::~CJX_Ui() = default;

void CJX_Ui::use(CFXJSE_Value* pValue,
                 bool bSetting,
                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Ui::usehref(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
