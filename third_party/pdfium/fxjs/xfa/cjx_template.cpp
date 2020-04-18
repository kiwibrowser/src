// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_template.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_template.h"

const CJX_MethodSpec CJX_Template::MethodSpecs[] = {
    {"execCalculate", execCalculate_static},
    {"execInitialize", execInitialize_static},
    {"execValidate", execValidate_static},
    {"formNodes", formNodes_static},
    {"recalculate", recalculate_static},
    {"remerge", remerge_static}};

CJX_Template::CJX_Template(CXFA_Template* tmpl) : CJX_Model(tmpl) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Template::~CJX_Template() {}

CJS_Return CJX_Template::formNodes(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(true));
}

CJS_Return CJX_Template::remerge(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  GetDocument()->DoDataRemerge(true);
  return CJS_Return(true);
}

CJS_Return CJX_Template::execInitialize(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(GetXFANode()->IsWidgetReady()));
}

CJS_Return CJX_Template::recalculate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(true));
}

CJS_Return CJX_Template::execCalculate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(GetXFANode()->IsWidgetReady()));
}

CJS_Return CJX_Template::execValidate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(GetXFANode()->IsWidgetReady()));
}
