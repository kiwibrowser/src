// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_logpseudomodel.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "xfa/fxfa/parser/cscript_logpseudomodel.h"

const CJX_MethodSpec CJX_LogPseudoModel::MethodSpecs[] = {
    {"message", message_static},
    {"traceEnabled", traceEnabled_static},
    {"traceActivate", traceActivate_static},
    {"traceDeactivate", traceDeactivate_static},
    {"trace", trace_static}};

CJX_LogPseudoModel::CJX_LogPseudoModel(CScript_LogPseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_LogPseudoModel::~CJX_LogPseudoModel() {}

CJS_Return CJX_LogPseudoModel::message(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_LogPseudoModel::traceEnabled(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_LogPseudoModel::traceActivate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_LogPseudoModel::traceDeactivate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_LogPseudoModel::trace(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}
