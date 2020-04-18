// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_encrypt.h"

#include "xfa/fxfa/parser/cxfa_encrypt.h"

CJX_Encrypt::CJX_Encrypt(CXFA_Encrypt* node) : CJX_Node(node) {}

CJX_Encrypt::~CJX_Encrypt() = default;

void CJX_Encrypt::use(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Encrypt::format(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {}

void CJX_Encrypt::usehref(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
