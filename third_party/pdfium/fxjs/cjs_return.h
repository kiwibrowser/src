// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_RETURN_H_
#define FXJS_CJS_RETURN_H_

#include "fxjs/cfxjs_engine.h"

class CJS_Return {
 public:
  explicit CJS_Return(bool);
  explicit CJS_Return(const WideString&);
  explicit CJS_Return(v8::Local<v8::Value>);
  CJS_Return(const CJS_Return&);
  ~CJS_Return();

  bool HasError() const { return is_error_; }
  WideString Error() const { return error_; }

  bool HasReturn() const { return !return_.IsEmpty(); }
  v8::Local<v8::Value> Return() const { return return_; }

 private:
  CJS_Return() = delete;

  bool is_error_ = false;
  WideString error_;
  v8::Local<v8::Value> return_;
};

#endif  // FXJS_CJS_RETURN_H_
