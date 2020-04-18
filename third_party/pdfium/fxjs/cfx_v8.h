// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CFX_V8_H_
#define FXJS_CFX_V8_H_

#include <map>
#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/unowned_ptr.h"
#include "v8/include/v8-util.h"
#include "v8/include/v8.h"

class CFX_V8 {
 public:
  explicit CFX_V8(v8::Isolate* pIsolate);
  virtual ~CFX_V8();

  v8::Isolate* GetIsolate() const { return m_isolate; }

  v8::Local<v8::Value> NewNull();
  v8::Local<v8::Value> NewUndefined();
  v8::Local<v8::Array> NewArray();
  v8::Local<v8::Object> NewObject();
  v8::Local<v8::Number> NewNumber(int number);
  v8::Local<v8::Number> NewNumber(double number);
  v8::Local<v8::Number> NewNumber(float number);
  v8::Local<v8::Boolean> NewBoolean(bool b);
  v8::Local<v8::String> NewString(const ByteStringView& str);
  v8::Local<v8::String> NewString(const WideStringView& str);
  v8::Local<v8::Date> NewDate(double d);

  int ToInt32(v8::Local<v8::Value> pValue);
  bool ToBoolean(v8::Local<v8::Value> pValue);
  double ToDouble(v8::Local<v8::Value> pValue);
  WideString ToWideString(v8::Local<v8::Value> pValue);
  ByteString ToByteString(v8::Local<v8::Value> pValue);
  v8::Local<v8::Object> ToObject(v8::Local<v8::Value> pValue);
  v8::Local<v8::Array> ToArray(v8::Local<v8::Value> pValue);

  // Arrays.
  unsigned GetArrayLength(v8::Local<v8::Array> pArray);
  v8::Local<v8::Value> GetArrayElement(v8::Local<v8::Array> pArray,
                                       unsigned index);
  unsigned PutArrayElement(v8::Local<v8::Array> pArray,
                           unsigned index,
                           v8::Local<v8::Value> pValue);

  // Objects.
  std::vector<WideString> GetObjectPropertyNames(v8::Local<v8::Object> pObj);
  v8::Local<v8::Value> GetObjectProperty(v8::Local<v8::Object> pObj,
                                         const WideString& PropertyName);
  void PutObjectProperty(v8::Local<v8::Object> pObj,
                         const WideString& PropertyName,
                         v8::Local<v8::Value> pValue);

 protected:
  void SetIsolate(v8::Isolate* pIsolate) { m_isolate = pIsolate; }

 private:
  v8::Isolate* m_isolate;
};

class CFX_V8ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
  static const size_t kMaxAllowedBytes = 0x10000000;
  void* Allocate(size_t length) override;
  void* AllocateUninitialized(size_t length) override;
  void Free(void* data, size_t length) override;
};

#endif  // FXJS_CFX_V8_H_
