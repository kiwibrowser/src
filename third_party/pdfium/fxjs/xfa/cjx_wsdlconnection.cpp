// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_wsdlconnection.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_wsdlconnection.h"

const CJX_MethodSpec CJX_WsdlConnection::MethodSpecs[] = {
    {"execute", execute_static}};

CJX_WsdlConnection::CJX_WsdlConnection(CXFA_WsdlConnection* connection)
    : CJX_Node(connection) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_WsdlConnection::~CJX_WsdlConnection() {}

CJS_Return CJX_WsdlConnection::execute(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty() && params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(false));
}

void CJX_WsdlConnection::dataDescription(CFXJSE_Value* pValue,
                                         bool bSetting,
                                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_WsdlConnection::execute(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
