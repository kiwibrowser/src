// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_UTIL_H_
#define FXJS_CJS_UTIL_H_

#include <string>
#include <vector>

#include "fxjs/JS_Define.h"

// Return values for ParseDataType() below.
#define UTIL_INT 0
#define UTIL_DOUBLE 1
#define UTIL_STRING 2

class CJS_Util : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_Util(v8::Local<v8::Object> pObject);
  ~CJS_Util() override;

  static WideString printx(const WideString& cFormat,
                           const WideString& cSource);

  JS_STATIC_METHOD(printd, CJS_Util);
  JS_STATIC_METHOD(printf, CJS_Util);
  JS_STATIC_METHOD(printx, CJS_Util);
  JS_STATIC_METHOD(scand, CJS_Util);
  JS_STATIC_METHOD(byteToChar, CJS_Util);

 private:
  friend class CJS_Util_ParseDataType_Test;

  static int ObjDefnID;
  static const char kName[];
  static const JSMethodSpec MethodSpecs[];

  static int ParseDataType(std::wstring* sFormat);

  CJS_Return printd(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return printf(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return printx(CJS_Runtime* pRuntime,
                    const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return scand(CJS_Runtime* pRuntime,
                   const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return byteToChar(CJS_Runtime* pRuntime,
                        const std::vector<v8::Local<v8::Value>>& params);
};

#endif  // FXJS_CJS_UTIL_H_
