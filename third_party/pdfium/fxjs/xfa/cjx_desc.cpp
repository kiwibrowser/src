// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_desc.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_desc.h"

const CJX_MethodSpec CJX_Desc::MethodSpecs[] = {{"metadata", metadata_static}};

CJX_Desc::CJX_Desc(CXFA_Desc* desc) : CJX_Node(desc) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Desc::~CJX_Desc() {}

CJS_Return CJX_Desc::metadata(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 0 && params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewString(""));
}

void CJX_Desc::use(CFXJSE_Value* pValue,
                   bool bSetting,
                   XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Desc::usehref(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
