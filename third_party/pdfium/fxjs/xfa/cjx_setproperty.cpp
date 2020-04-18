// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_setproperty.h"

#include "xfa/fxfa/parser/cxfa_setproperty.h"

CJX_SetProperty::CJX_SetProperty(CXFA_SetProperty* node) : CJX_Node(node) {}

CJX_SetProperty::~CJX_SetProperty() = default;

void CJX_SetProperty::connection(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SetProperty::target(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
