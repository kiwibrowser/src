// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_REPORT_H_
#define FXJS_CJS_REPORT_H_

#include <vector>

#include "fxjs/JS_Define.h"

class CJS_Report : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine, FXJSOBJTYPE eObjType);

  explicit CJS_Report(v8::Local<v8::Object> pObject);
  ~CJS_Report() override;

  JS_STATIC_METHOD(save, CJS_Report);
  JS_STATIC_METHOD(writeText, CJS_Report);

 private:
  static int ObjDefnID;
  static const char kName[];
  static const JSMethodSpec MethodSpecs[];

  CJS_Return save(CJS_Runtime* pRuntime,
                  const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return writeText(CJS_Runtime* pRuntime,
                       const std::vector<v8::Local<v8::Value>>& params);
};

#endif  // FXJS_CJS_REPORT_H_
