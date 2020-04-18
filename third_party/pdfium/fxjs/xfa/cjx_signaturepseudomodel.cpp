// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_signaturepseudomodel.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cscript_signaturepseudomodel.h"

const CJX_MethodSpec CJX_SignaturePseudoModel::MethodSpecs[] = {
    {"verify", verifySignature_static},
    {"sign", sign_static},
    {"enumerate", enumerate_static},
    {"clear", clear_static}};

CJX_SignaturePseudoModel::CJX_SignaturePseudoModel(
    CScript_SignaturePseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_SignaturePseudoModel::~CJX_SignaturePseudoModel() {}

CJS_Return CJX_SignaturePseudoModel::verifySignature(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 4)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_SignaturePseudoModel::sign(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() < 3 || params.size() > 7)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(false));
}

CJS_Return CJX_SignaturePseudoModel::enumerate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_SignaturePseudoModel::clear(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewBoolean(false));
}
