// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_container.h"

#include <vector>

#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "xfa/fxfa/parser/cxfa_arraynodelist.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_field.h"

const CJX_MethodSpec CJX_Container::MethodSpecs[] = {
    {"getDelta", getDelta_static},
    {"getDeltas", getDeltas_static}};

CJX_Container::CJX_Container(CXFA_Node* node) : CJX_Node(node) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Container::~CJX_Container() {}

CJS_Return CJX_Container::getDelta(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_Container::getDeltas(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_ArrayNodeList* pFormNodes = new CXFA_ArrayNodeList(GetDocument());
  return CJS_Return(static_cast<CFXJSE_Engine*>(runtime)->NewXFAObject(
      pFormNodes,
      GetDocument()->GetScriptContext()->GetJseNormalClass()->GetTemplate()));
}
