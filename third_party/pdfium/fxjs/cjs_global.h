// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_GLOBAL_H_
#define FXJS_CJS_GLOBAL_H_

#include <map>
#include <memory>
#include <vector>

#include "fxjs/JS_Define.h"
#include "fxjs/cjs_keyvalue.h"

class CJS_GlobalData;

class CJS_Global : public CJS_Object {
 public:
  static void DefineJSObjects(CFXJS_Engine* pEngine);
  static void DefineAllProperties(CFXJS_Engine* pEngine);

  static void queryprop_static(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Integer>& info);
  static void getprop_static(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value>& info);
  static void putprop_static(v8::Local<v8::Name> property,
                             v8::Local<v8::Value> value,
                             const v8::PropertyCallbackInfo<v8::Value>& info);
  static void delprop_static(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Boolean>& info);

  static void setPersistent_static(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  explicit CJS_Global(v8::Local<v8::Object> pObject);
  ~CJS_Global() override;

  // CJS_Object
  void InitInstance(IJS_Runtime* pIRuntime) override;

  CJS_Return DelProperty(CJS_Runtime* pRuntime, const wchar_t* propname);
  void Initial(CPDFSDK_FormFillEnvironment* pFormFillEnv);

  CJS_Return setPersistent(CJS_Runtime* pRuntime,
                           const std::vector<v8::Local<v8::Value>>& params);
  CJS_Return QueryProperty(const wchar_t* propname);
  CJS_Return GetProperty(CJS_Runtime* pRuntime, const wchar_t* propname);
  CJS_Return SetProperty(CJS_Runtime* pRuntime,
                         const wchar_t* propname,
                         v8::Local<v8::Value> vp);

 private:
  struct JSGlobalData {
    JSGlobalData();
    ~JSGlobalData();

    JS_GlobalDataType nType;
    double dData;
    bool bData;
    ByteString sData;
    v8::Global<v8::Object> pData;
    bool bPersistent;
    bool bDeleted;
  };

  static int ObjDefnID;
  static const JSMethodSpec MethodSpecs[];

  void UpdateGlobalPersistentVariables();
  void CommitGlobalPersisitentVariables(CJS_Runtime* pRuntime);
  void DestroyGlobalPersisitentVariables();
  CJS_Return SetGlobalVariables(const ByteString& propname,
                                JS_GlobalDataType nType,
                                double dData,
                                bool bData,
                                const ByteString& sData,
                                v8::Local<v8::Object> pData,
                                bool bDefaultPersistent);
  void ObjectToArray(CJS_Runtime* pRuntime,
                     v8::Local<v8::Object> pObj,
                     CJS_GlobalVariableArray& array);
  void PutObjectProperty(v8::Local<v8::Object> obj, CJS_KeyValue* pData);

  std::map<ByteString, std::unique_ptr<JSGlobalData>> m_MapGlobal;
  WideString m_sFilePath;
  CJS_GlobalData* m_pGlobalData;
  CPDFSDK_FormFillEnvironment::ObservedPtr m_pFormFillEnv;
};

#endif  // FXJS_CJS_GLOBAL_H_
