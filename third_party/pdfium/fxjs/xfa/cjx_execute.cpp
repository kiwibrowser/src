// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_execute.h"

#include "xfa/fxfa/parser/cxfa_execute.h"

CJX_Execute::CJX_Execute(CXFA_Execute* node) : CJX_Node(node) {}

CJX_Execute::~CJX_Execute() = default;

void CJX_Execute::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Execute::connection(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Execute::runAt(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Execute::executeType(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Execute::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
