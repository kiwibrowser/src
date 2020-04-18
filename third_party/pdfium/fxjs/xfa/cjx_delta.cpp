// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_delta.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_delta.h"

const CJX_MethodSpec CJX_Delta::MethodSpecs[] = {{"restore", restore_static}};

CJX_Delta::CJX_Delta(CXFA_Delta* delta) : CJX_Object(delta) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Delta::~CJX_Delta() {}

CJS_Return CJX_Delta::restore(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

void CJX_Delta::currentValue(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {}

void CJX_Delta::savedValue(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {}

void CJX_Delta::target(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {}
