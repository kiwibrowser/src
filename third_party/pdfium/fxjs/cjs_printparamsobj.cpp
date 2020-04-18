// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_printparamsobj.h"

int CJS_PrintParamsObj::ObjDefnID = -1;

// static
int CJS_PrintParamsObj::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_PrintParamsObj::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID =
      pEngine->DefineObj("PrintParamsObj", FXJSOBJTYPE_DYNAMIC,
                         JSConstructor<CJS_PrintParamsObj>, JSDestructor);
}

CJS_PrintParamsObj::CJS_PrintParamsObj(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject) {}

CJS_PrintParamsObj::~CJS_PrintParamsObj() = default;
