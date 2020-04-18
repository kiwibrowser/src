// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_PRINTPARAMSOBJ_H_
#define FXJS_CJS_PRINTPARAMSOBJ_H_

#include "fxjs/JS_Define.h"

class CJS_PrintParamsObj : public CJS_Object {
 public:
  static int GetObjDefnID();
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_PrintParamsObj(v8::Local<v8::Object> pObject);
  ~CJS_PrintParamsObj() override;

  bool GetUI() const { return bUI; }
  int GetStart() const { return nStart; }
  int GetEnd() const { return nEnd; }
  bool GetSilent() const { return bSilent; }
  bool GetShrinkToFit() const { return bShrinkToFit; }
  bool GetPrintAsImage() const { return bPrintAsImage; }
  bool GetReverse() const { return bReverse; }
  bool GetAnnotations() const { return bAnnotations; }

 private:
  static int ObjDefnID;

  bool bUI = true;
  int nStart = 0;
  int nEnd = 0;
  bool bSilent = false;
  bool bShrinkToFit = false;
  bool bPrintAsImage = false;
  bool bReverse = false;
  bool bAnnotations = true;
};

#endif  // FXJS_CJS_PRINTPARAMSOBJ_H_
