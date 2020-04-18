// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJX_DEFINE_H_
#define FXJS_CJX_DEFINE_H_

#include <vector>

#include "fxjs/cfx_v8.h"
#include "fxjs/cjs_return.h"

template <class C,
          CJS_Return (C::*M)(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params)>
CJS_Return JSMethod(C* node,
                    CFX_V8* runtime,
                    const std::vector<v8::Local<v8::Value>>& params) {
  return (node->*M)(runtime, params);
}

#define JS_METHOD(method_name, class_name)                 \
  static CJS_Return method_name##_static(                  \
      CJX_Object* node, CFX_V8* runtime,                   \
      const std::vector<v8::Local<v8::Value>>& params) {   \
    return JSMethod<class_name, &class_name::method_name>( \
        static_cast<class_name*>(node), runtime, params);  \
  }                                                        \
  CJS_Return method_name(CFX_V8* runtime,                  \
                         const std::vector<v8::Local<v8::Value>>& params)

#define JS_PROP(prop_name) \
  void prop_name(CFXJSE_Value* pValue, bool bSetting, XFA_Attribute eAttribute)

#endif  // FXJS_CJX_DEFINE_H_
