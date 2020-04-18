// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_xsdconnection.h"

#include "xfa/fxfa/parser/cxfa_xsdconnection.h"

CJX_XsdConnection::CJX_XsdConnection(CXFA_XsdConnection* node)
    : CJX_Node(node) {}

CJX_XsdConnection::~CJX_XsdConnection() = default;

void CJX_XsdConnection::dataDescription(CFXJSE_Value* pValue,
                                        bool bSetting,
                                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
