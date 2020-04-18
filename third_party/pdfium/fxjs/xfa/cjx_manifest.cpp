// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_manifest.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_manifest.h"

const CJX_MethodSpec CJX_Manifest::MethodSpecs[] = {
    {"evaluate", evaluate_static}};

CJX_Manifest::CJX_Manifest(CXFA_Manifest* manifest) : CJX_Node(manifest) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Manifest::~CJX_Manifest() {}

CJS_Return CJX_Manifest::evaluate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  return CJS_Return(runtime->NewBoolean(GetXFANode()->IsWidgetReady()));
}

void CJX_Manifest::defaultValue(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  Script_Som_DefaultValue(pValue, bSetting, eAttribute);
}

void CJX_Manifest::action(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Manifest::use(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Manifest::usehref(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
