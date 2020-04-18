// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_oid.h"

#include "xfa/fxfa/parser/cxfa_oid.h"

CJX_Oid::CJX_Oid(CXFA_Oid* node) : CJX_TextNode(node) {}

CJX_Oid::~CJX_Oid() = default;

void CJX_Oid::use(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Oid::usehref(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
