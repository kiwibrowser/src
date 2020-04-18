// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_ZOOMTYPE_H_
#define FXJS_CJS_ZOOMTYPE_H_

#include "fxjs/JS_Define.h"

class CJS_Zoomtype : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_Zoomtype(v8::Local<v8::Object> pObject) : CJS_Object(pObject) {}
  ~CJS_Zoomtype() override {}

 private:
  static int ObjDefnID;
  static const JSConstSpec ConstSpecs[];
};

#endif  // FXJS_CJS_ZOOMTYPE_H_
