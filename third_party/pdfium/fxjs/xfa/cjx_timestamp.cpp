// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_timestamp.h"

#include "xfa/fxfa/parser/cxfa_timestamp.h"

CJX_TimeStamp::CJX_TimeStamp(CXFA_TimeStamp* node) : CJX_Node(node) {}

CJX_TimeStamp::~CJX_TimeStamp() = default;

void CJX_TimeStamp::use(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_TimeStamp::type(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_TimeStamp::server(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_TimeStamp::usehref(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
