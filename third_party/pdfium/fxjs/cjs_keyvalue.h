// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_KEYVALUE_H_
#define FXJS_CJS_KEYVALUE_H_

#include "core/fxcrt/fx_string.h"
#include "fxjs/cjs_globalvariablearray.h"

enum class JS_GlobalDataType { NUMBER = 0, BOOLEAN, STRING, OBJECT, NULLOBJ };

class CJS_KeyValue {
 public:
  CJS_KeyValue();
  ~CJS_KeyValue();

  ByteString sKey;
  JS_GlobalDataType nType;
  double dData;
  bool bData;
  ByteString sData;
  CJS_GlobalVariableArray objData;
};

#endif  // FXJS_CJS_KEYVALUE_H_
