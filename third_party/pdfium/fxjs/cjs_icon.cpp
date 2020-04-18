// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_icon.h"

const JSPropertySpec CJS_Icon::PropertySpecs[] = {
    {"name", get_name_static, set_name_static}};

int CJS_Icon::ObjDefnID = -1;
const char CJS_Icon::kName[] = "Icon";

// static
int CJS_Icon::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_Icon::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Icon::kName, FXJSOBJTYPE_DYNAMIC,
                                 JSConstructor<CJS_Icon>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs, FX_ArraySize(PropertySpecs));
}

CJS_Icon::CJS_Icon(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject), m_swIconName(L"") {}

CJS_Icon::~CJS_Icon() = default;

CJS_Return CJS_Icon::get_name(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewString(m_swIconName.c_str()));
}

CJS_Return CJS_Icon::set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp) {
  return CJS_Return(false);
}
