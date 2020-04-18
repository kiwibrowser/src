// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_handler.h"

#include "xfa/fxfa/parser/cxfa_handler.h"

CJX_Handler::CJX_Handler(CXFA_Handler* node) : CJX_TextNode(node) {}

CJX_Handler::~CJX_Handler() = default;

void CJX_Handler::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Handler::type(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Handler::version(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {}

void CJX_Handler::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
