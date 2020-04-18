// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_exdata.h"

#include "xfa/fxfa/parser/cxfa_exdata.h"

CJX_ExData::CJX_ExData(CXFA_ExData* node) : CJX_Content(node) {}

CJX_ExData::~CJX_ExData() = default;

void CJX_ExData::use(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExData::contentType(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExData::transferEncoding(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExData::defaultValue(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Som_DefaultValue(pValue, bSetting, eAttribute);
}

void CJX_ExData::usehref(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExData::maxLength(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExData::href(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
