// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_soapaction.h"

#include "xfa/fxfa/parser/cxfa_soapaction.h"

CJX_SoapAction::CJX_SoapAction(CXFA_SoapAction* node) : CJX_TextNode(node) {}

CJX_SoapAction::~CJX_SoapAction() = default;

void CJX_SoapAction::use(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SoapAction::usehref(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
