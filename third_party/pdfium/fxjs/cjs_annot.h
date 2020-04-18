// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_ANNOT_H_
#define FXJS_CJS_ANNOT_H_

#include "fpdfsdk/cpdfsdk_baannot.h"
#include "fxjs/JS_Define.h"

class CJS_Annot : public CJS_Object {
 public:
  static int GetObjDefnID();
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  explicit CJS_Annot(v8::Local<v8::Object> pObject);
  ~CJS_Annot() override;

  void SetSDKAnnot(CPDFSDK_BAAnnot* annot) { m_pAnnot.Reset(annot); }

  JS_STATIC_PROP(hidden, hidden, CJS_Annot);
  JS_STATIC_PROP(name, name, CJS_Annot);
  JS_STATIC_PROP(type, type, CJS_Annot);

 private:
  static int ObjDefnID;
  static const char kName[];
  static const JSPropertySpec PropertySpecs[];

  CJS_Return get_hidden(CJS_Runtime* pRuntime);
  CJS_Return set_hidden(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_name(CJS_Runtime* pRuntime);
  CJS_Return set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Return get_type(CJS_Runtime* pRuntime);
  CJS_Return set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CPDFSDK_Annot::ObservedPtr m_pAnnot;
};

#endif  // FXJS_CJS_ANNOT_H_
