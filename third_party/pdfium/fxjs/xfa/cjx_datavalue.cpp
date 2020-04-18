// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_datavalue.h"

#include "xfa/fxfa/parser/cxfa_datavalue.h"

CJX_DataValue::CJX_DataValue(CXFA_DataValue* node) : CJX_Node(node) {}

CJX_DataValue::~CJX_DataValue() = default;

void CJX_DataValue::contentType(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_DataValue::contains(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_DataValue::defaultValue(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Som_DefaultValue(pValue, bSetting, eAttribute);
}

void CJX_DataValue::value(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  defaultValue(pValue, bSetting, eAttribute);
}

void CJX_DataValue::isNull(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_BOOL(pValue, bSetting, eAttribute);
}
