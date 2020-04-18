// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_GLOBALVARIABLEARRAY_H_
#define FXJS_CJS_GLOBALVARIABLEARRAY_H_

#include <memory>
#include <vector>

class CJS_KeyValue;

class CJS_GlobalVariableArray {
 public:
  CJS_GlobalVariableArray();
  ~CJS_GlobalVariableArray();

  void Add(CJS_KeyValue* p);
  int Count() const;
  CJS_KeyValue* GetAt(int index) const;
  void Copy(const CJS_GlobalVariableArray& array);

 private:
  std::vector<std::unique_ptr<CJS_KeyValue>> m_Array;
};

#endif  // FXJS_CJS_GLOBALVARIABLEARRAY_H_
