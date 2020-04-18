// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_signatureproperties.h"

#include "xfa/fxfa/parser/cxfa_signatureproperties.h"

CJX_SignatureProperties::CJX_SignatureProperties(CXFA_SignatureProperties* node)
    : CJX_Node(node) {}

CJX_SignatureProperties::~CJX_SignatureProperties() = default;

void CJX_SignatureProperties::use(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_SignatureProperties::usehref(CFXJSE_Value* pValue,
                                      bool bSetting,
                                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
