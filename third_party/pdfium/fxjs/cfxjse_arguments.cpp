// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjse_arguments.h"

#include "fxjs/cfxjse_context.h"
#include "fxjs/cfxjse_value.h"
#include "third_party/base/ptr_util.h"

CFXJSE_Arguments::CFXJSE_Arguments(
    const v8::FunctionCallbackInfo<v8::Value>* pInfo,
    CFXJSE_Value* pRetValue)
    : m_pInfo(pInfo), m_pRetValue(pRetValue) {}

CFXJSE_Arguments::~CFXJSE_Arguments() {}

int32_t CFXJSE_Arguments::GetLength() const {
  return m_pInfo->Length();
}

std::unique_ptr<CFXJSE_Value> CFXJSE_Arguments::GetValue(int32_t index) const {
  auto pArgValue = pdfium::MakeUnique<CFXJSE_Value>(v8::Isolate::GetCurrent());
  pArgValue->ForceSetValue((*m_pInfo)[index]);
  return pArgValue;
}

bool CFXJSE_Arguments::GetBoolean(int32_t index) const {
  return (*m_pInfo)[index]->BooleanValue();
}

int32_t CFXJSE_Arguments::GetInt32(int32_t index) const {
  return static_cast<int32_t>((*m_pInfo)[index]->NumberValue());
}

float CFXJSE_Arguments::GetFloat(int32_t index) const {
  return static_cast<float>((*m_pInfo)[index]->NumberValue());
}

ByteString CFXJSE_Arguments::GetUTF8String(int32_t index) const {
  v8::Local<v8::String> hString = (*m_pInfo)[index]->ToString();
  v8::String::Utf8Value szStringVal(m_pInfo->GetIsolate(), hString);
  return ByteString(*szStringVal);
}

CFXJSE_HostObject* CFXJSE_Arguments::GetObject(int32_t index,
                                               CFXJSE_Class* pClass) const {
  v8::Local<v8::Value> hValue = (*m_pInfo)[index];
  ASSERT(!hValue.IsEmpty());
  if (!hValue->IsObject())
    return nullptr;
  return FXJSE_RetrieveObjectBinding(hValue.As<v8::Object>(), pClass);
}

CFXJSE_Value* CFXJSE_Arguments::GetReturnValue() const {
  return m_pRetValue.Get();
}
