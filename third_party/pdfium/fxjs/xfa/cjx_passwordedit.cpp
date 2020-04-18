// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_passwordedit.h"

#include "xfa/fxfa/parser/cxfa_passwordedit.h"

CJX_PasswordEdit::CJX_PasswordEdit(CXFA_PasswordEdit* node) : CJX_Node(node) {}

CJX_PasswordEdit::~CJX_PasswordEdit() = default;

void CJX_PasswordEdit::use(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PasswordEdit::passwordChar(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PasswordEdit::usehref(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_PasswordEdit::hScrollPolicy(CFXJSE_Value* pValue,
                                     bool bSetting,
                                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
