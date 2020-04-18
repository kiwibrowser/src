// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_source.h"

#include <vector>

#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_source.h"

const CJX_MethodSpec CJX_Source::MethodSpecs[] = {
    {"addNew", addNew_static},
    {"cancel", cancel_static},
    {"cancelBatch", cancelBatch_static},
    {"close", close_static},
    {"delete", deleteItem_static},
    {"first", first_static},
    {"hasDataChanged", hasDataChanged_static},
    {"isBOF", isBOF_static},
    {"isEOF", isEOF_static},
    {"last", last_static},
    {"next", next_static},
    {"open", open_static},
    {"previous", previous_static},
    {"requery", requery_static},
    {"resync", resync_static},
    {"update", update_static},
    {"updateBatch", updateBatch_static}};

CJX_Source::CJX_Source(CXFA_Source* src) : CJX_Node(src) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Source::~CJX_Source() {}

CJS_Return CJX_Source::next(CFX_V8* runtime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::cancelBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::first(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::updateBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::previous(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::isBOF(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::isEOF(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::cancel(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::update(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::open(CFX_V8* runtime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::deleteItem(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::addNew(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::requery(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::resync(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::close(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::last(CFX_V8* runtime,
                            const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

CJS_Return CJX_Source::hasDataChanged(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(true);
}

void CJX_Source::db(CFXJSE_Value* pValue,
                    bool bSetting,
                    XFA_Attribute eAttribute) {}

void CJX_Source::use(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Source::usehref(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
