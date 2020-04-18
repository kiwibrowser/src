// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_JS_DEFINE_H_
#define FXJS_JS_DEFINE_H_

#include <utility>
#include <vector>

#include "fxjs/cfxjs_engine.h"
#include "fxjs/cjs_object.h"
#include "fxjs/cjs_return.h"
#include "fxjs/js_resources.h"
#include "third_party/base/ptr_util.h"

double JS_GetDateTime();
int JS_GetYearFromTime(double dt);
int JS_GetMonthFromTime(double dt);
int JS_GetDayFromTime(double dt);
int JS_GetHourFromTime(double dt);
int JS_GetMinFromTime(double dt);
int JS_GetSecFromTime(double dt);
double JS_LocalTime(double d);
double JS_DateParse(const WideString& str);
double JS_MakeDay(int nYear, int nMonth, int nDay);
double JS_MakeTime(int nHour, int nMin, int nSec, int nMs);
double JS_MakeDate(double day, double time);

// Some JS methods have the bizarre convention that they may also be called
// with a single argument which is an object containing the actual arguments
// as its properties. The varying arguments to this method are the property
// names as wchar_t string literals corresponding to each positional argument.
// The result will always contain |nKeywords| value, with unspecified ones
// being set to type VT_unknown.
std::vector<v8::Local<v8::Value>> ExpandKeywordParams(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& originals,
    size_t nKeywords,
    ...);

// All JS classes have a name, an object defintion ID, and the ability to
// register themselves with FXJS_V8. We never make a BASE class on its own
// because it can't really do anything.

// Rich JS classes provide constants, methods, properties, and the ability
// to construct native object state.

template <class T>
static void JSConstructor(CFXJS_Engine* pEngine, v8::Local<v8::Object> obj) {
  auto pObj = pdfium::MakeUnique<T>(obj);
  pObj->InitInstance(static_cast<CJS_Runtime*>(pEngine));
  pEngine->SetObjectPrivate(obj, std::move(pObj));
}

// CJS_Object has vitual dtor, template not required.
void JSDestructor(v8::Local<v8::Object> obj);

template <class C, CJS_Return (C::*M)(CJS_Runtime*)>
void JSPropGetter(const char* prop_name_string,
                  const char* class_name_string,
                  v8::Local<v8::String> property,
                  const v8::PropertyCallbackInfo<v8::Value>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  C* pObj = static_cast<C*>(pJSObj);
  CJS_Return result = (pObj->*M)(pRuntime);
  if (result.HasError()) {
    pRuntime->Error(JSFormatErrorString(class_name_string, prop_name_string,
                                        result.Error()));
    return;
  }

  if (result.HasReturn())
    info.GetReturnValue().Set(result.Return());
}

template <class C, CJS_Return (C::*M)(CJS_Runtime*, v8::Local<v8::Value>)>
void JSPropSetter(const char* prop_name_string,
                  const char* class_name_string,
                  v8::Local<v8::String> property,
                  v8::Local<v8::Value> value,
                  const v8::PropertyCallbackInfo<void>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  C* pObj = static_cast<C*>(pJSObj);
  CJS_Return result = (pObj->*M)(pRuntime, value);
  if (result.HasError()) {
    pRuntime->Error(JSFormatErrorString(class_name_string, prop_name_string,
                                        result.Error()));
  }
}

template <class C,
          CJS_Return (C::*M)(CJS_Runtime*,
                             const std::vector<v8::Local<v8::Value>>&)>
void JSMethod(const char* method_name_string,
              const char* class_name_string,
              const v8::FunctionCallbackInfo<v8::Value>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  std::vector<v8::Local<v8::Value>> parameters;
  for (unsigned int i = 0; i < (unsigned int)info.Length(); i++)
    parameters.push_back(info[i]);

  C* pObj = static_cast<C*>(pJSObj);
  CJS_Return result = (pObj->*M)(pRuntime, parameters);
  if (result.HasError()) {
    pRuntime->Error(JSFormatErrorString(class_name_string, method_name_string,
                                        result.Error()));
    return;
  }

  if (result.HasReturn())
    info.GetReturnValue().Set(result.Return());
}

#define JS_STATIC_PROP(err_name, prop_name, class_name)           \
  static void get_##prop_name##_static(                           \
      v8::Local<v8::String> property,                             \
      const v8::PropertyCallbackInfo<v8::Value>& info) {          \
    JSPropGetter<class_name, &class_name::get_##prop_name>(       \
        #err_name, class_name::kName, property, info);            \
  }                                                               \
  static void set_##prop_name##_static(                           \
      v8::Local<v8::String> property, v8::Local<v8::Value> value, \
      const v8::PropertyCallbackInfo<void>& info) {               \
    JSPropSetter<class_name, &class_name::set_##prop_name>(       \
        #err_name, class_name::kName, property, value, info);     \
  }

#define JS_STATIC_METHOD(method_name, class_name)                            \
  static void method_name##_static(                                          \
      const v8::FunctionCallbackInfo<v8::Value>& info) {                     \
    JSMethod<class_name, &class_name::method_name>(#method_name,             \
                                                   class_name::kName, info); \
  }

#endif  // FXJS_JS_DEFINE_H_
