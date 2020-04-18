// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_global.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/fx_extension.h"
#include "fxjs/JS_Define.h"
#include "fxjs/cjs_event_context.h"
#include "fxjs/cjs_eventhandler.h"
#include "fxjs/cjs_globaldata.h"
#include "fxjs/cjs_keyvalue.h"
#include "fxjs/cjs_object.h"
#include "fxjs/js_resources.h"

namespace {

WideString PropFromV8Prop(v8::Isolate* pIsolate,
                          v8::Local<v8::String> property) {
  v8::String::Utf8Value utf8_value(pIsolate, property);
  return WideString::FromUTF8(ByteStringView(*utf8_value, utf8_value.length()));
}

template <class Alt>
void JSSpecialPropQuery(const char*,
                        v8::Local<v8::String> property,
                        const v8::PropertyCallbackInfo<v8::Integer>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  Alt* pObj = static_cast<Alt*>(pJSObj);
  CJS_Return result =
      pObj->QueryProperty(PropFromV8Prop(info.GetIsolate(), property).c_str());
  info.GetReturnValue().Set(!result.HasError() ? 4 : 0);
}

template <class Alt>
void JSSpecialPropGet(const char* class_name,
                      v8::Local<v8::String> property,
                      const v8::PropertyCallbackInfo<v8::Value>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  Alt* pObj = static_cast<Alt*>(pJSObj);
  CJS_Return result = pObj->GetProperty(
      pRuntime, PropFromV8Prop(info.GetIsolate(), property).c_str());
  if (result.HasError()) {
    pRuntime->Error(
        JSFormatErrorString(class_name, "GetProperty", result.Error()));
    return;
  }

  if (result.HasReturn())
    info.GetReturnValue().Set(result.Return());
}

template <class Alt>
void JSSpecialPropPut(const char* class_name,
                      v8::Local<v8::String> property,
                      v8::Local<v8::Value> value,
                      const v8::PropertyCallbackInfo<v8::Value>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  Alt* pObj = static_cast<Alt*>(pJSObj);
  CJS_Return result = pObj->SetProperty(
      pRuntime, PropFromV8Prop(info.GetIsolate(), property).c_str(), value);
  if (result.HasError()) {
    pRuntime->Error(
        JSFormatErrorString(class_name, "PutProperty", result.Error()));
  }
}

template <class Alt>
void JSSpecialPropDel(const char* class_name,
                      v8::Local<v8::String> property,
                      const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(info.GetIsolate());
  if (!pRuntime)
    return;

  CJS_Object* pJSObj = pRuntime->GetObjectPrivate(info.Holder());
  if (!pJSObj)
    return;

  Alt* pObj = static_cast<Alt*>(pJSObj);
  CJS_Return result = pObj->DelProperty(
      pRuntime, PropFromV8Prop(info.GetIsolate(), property).c_str());
  if (result.HasError()) {
    // TODO(dsinclair): Should this set the pRuntime->Error result?
    // ByteString cbName =
    //     ByteString::Format("%s.%s", class_name, "DelProperty");
  }
}

}  // namespace

CJS_Global::JSGlobalData::JSGlobalData()
    : nType(JS_GlobalDataType::NUMBER),
      dData(0),
      bData(false),
      sData(""),
      bPersistent(false),
      bDeleted(false) {}

CJS_Global::JSGlobalData::~JSGlobalData() {
  pData.Reset();
}

const JSMethodSpec CJS_Global::MethodSpecs[] = {
    {"setPersistent", setPersistent_static}};

int CJS_Global::ObjDefnID = -1;

// static
void CJS_Global::setPersistent_static(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  JSMethod<CJS_Global, &CJS_Global::setPersistent>("setPersistent", "global",
                                                   info);
}

// static
void CJS_Global::queryprop_static(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Integer>& info) {
  DCHECK(property->IsString());
  JSSpecialPropQuery<CJS_Global>(
      "global",
      v8::Local<v8::String>::New(info.GetIsolate(), property->ToString()),
      info);
}

// static
void CJS_Global::getprop_static(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  DCHECK(property->IsString());
  JSSpecialPropGet<CJS_Global>(
      "global",
      v8::Local<v8::String>::New(info.GetIsolate(), property->ToString()),
      info);
}

// static
void CJS_Global::putprop_static(
    v8::Local<v8::Name> property,
    v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  DCHECK(property->IsString());
  JSSpecialPropPut<CJS_Global>(
      "global",
      v8::Local<v8::String>::New(info.GetIsolate(), property->ToString()),
      value, info);
}

// static
void CJS_Global::delprop_static(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  DCHECK(property->IsString());
  JSSpecialPropDel<CJS_Global>(
      "global",
      v8::Local<v8::String>::New(info.GetIsolate(), property->ToString()),
      info);
}

// static
void CJS_Global::DefineAllProperties(CFXJS_Engine* pEngine) {
  pEngine->DefineObjAllProperties(
      ObjDefnID, CJS_Global::queryprop_static, CJS_Global::getprop_static,
      CJS_Global::putprop_static, CJS_Global::delprop_static);
}

// static
void CJS_Global::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj("global", FXJSOBJTYPE_STATIC,
                                 JSConstructor<CJS_Global>, JSDestructor);
  DefineMethods(pEngine, ObjDefnID, MethodSpecs, FX_ArraySize(MethodSpecs));
  DefineAllProperties(pEngine);
}

CJS_Global::CJS_Global(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject), m_pFormFillEnv(nullptr) {}

CJS_Global::~CJS_Global() {
  DestroyGlobalPersisitentVariables();
  m_pGlobalData->Release();
}

void CJS_Global::InitInstance(IJS_Runtime* pIRuntime) {
  Initial(pIRuntime->GetFormFillEnv());
}

void CJS_Global::Initial(CPDFSDK_FormFillEnvironment* pFormFillEnv) {
  m_pFormFillEnv.Reset(pFormFillEnv);
  m_pGlobalData = CJS_GlobalData::GetRetainedInstance(pFormFillEnv);
  UpdateGlobalPersistentVariables();
}

CJS_Return CJS_Global::QueryProperty(const wchar_t* propname) {
  return CJS_Return(WideString(propname) != L"setPersistent");
}

CJS_Return CJS_Global::DelProperty(CJS_Runtime* pRuntime,
                                   const wchar_t* propname) {
  auto it = m_MapGlobal.find(ByteString::FromUnicode(propname));
  if (it == m_MapGlobal.end())
    return CJS_Return(false);

  it->second->bDeleted = true;
  return CJS_Return(true);
}

CJS_Return CJS_Global::GetProperty(CJS_Runtime* pRuntime,
                                   const wchar_t* propname) {
  auto it = m_MapGlobal.find(ByteString::FromUnicode(propname));
  if (it == m_MapGlobal.end())
    return CJS_Return(true);

  JSGlobalData* pData = it->second.get();
  if (pData->bDeleted)
    return CJS_Return(true);

  switch (pData->nType) {
    case JS_GlobalDataType::NUMBER:
      return CJS_Return(pRuntime->NewNumber(pData->dData));
    case JS_GlobalDataType::BOOLEAN:
      return CJS_Return(pRuntime->NewBoolean(pData->bData));
    case JS_GlobalDataType::STRING:
      return CJS_Return(pRuntime->NewString(
          WideString::FromLocal(pData->sData.c_str()).c_str()));
    case JS_GlobalDataType::OBJECT:
      return CJS_Return(
          v8::Local<v8::Object>::New(pRuntime->GetIsolate(), pData->pData));
    case JS_GlobalDataType::NULLOBJ:
      return CJS_Return(pRuntime->NewNull());
    default:
      break;
  }
  return CJS_Return(false);
}

CJS_Return CJS_Global::SetProperty(CJS_Runtime* pRuntime,
                                   const wchar_t* propname,
                                   v8::Local<v8::Value> vp) {
  ByteString sPropName = ByteString::FromUnicode(propname);
  if (vp->IsNumber()) {
    return SetGlobalVariables(sPropName, JS_GlobalDataType::NUMBER,
                              pRuntime->ToDouble(vp), false, "",
                              v8::Local<v8::Object>(), false);
  }
  if (vp->IsBoolean()) {
    return SetGlobalVariables(sPropName, JS_GlobalDataType::BOOLEAN, 0,
                              pRuntime->ToBoolean(vp), "",
                              v8::Local<v8::Object>(), false);
  }
  if (vp->IsString()) {
    return SetGlobalVariables(
        sPropName, JS_GlobalDataType::STRING, 0, false,
        ByteString::FromUnicode(pRuntime->ToWideString(vp)),
        v8::Local<v8::Object>(), false);
  }
  if (vp->IsObject()) {
    return SetGlobalVariables(sPropName, JS_GlobalDataType::OBJECT, 0, false,
                              "", pRuntime->ToObject(vp), false);
  }
  if (vp->IsNull()) {
    return SetGlobalVariables(sPropName, JS_GlobalDataType::NULLOBJ, 0, false,
                              "", v8::Local<v8::Object>(), false);
  }
  if (vp->IsUndefined()) {
    DelProperty(pRuntime, propname);
    return CJS_Return(true);
  }
  return CJS_Return(false);
}

CJS_Return CJS_Global::setPersistent(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  auto it = m_MapGlobal.find(
      ByteString::FromUnicode(pRuntime->ToWideString(params[0])));
  if (it == m_MapGlobal.end() || it->second->bDeleted)
    return CJS_Return(JSGetStringFromID(JSMessage::kGlobalNotFoundError));

  it->second->bPersistent = pRuntime->ToBoolean(params[1]);
  return CJS_Return(true);
}

void CJS_Global::UpdateGlobalPersistentVariables() {
  CJS_Runtime* pRuntime =
      static_cast<CJS_Runtime*>(CFXJS_Engine::EngineFromIsolateCurrentContext(
          ToV8Object()->GetIsolate()));

  for (int i = 0, sz = m_pGlobalData->GetSize(); i < sz; i++) {
    CJS_GlobalData_Element* pData = m_pGlobalData->GetAt(i);
    switch (pData->data.nType) {
      case JS_GlobalDataType::NUMBER:
        SetGlobalVariables(pData->data.sKey, JS_GlobalDataType::NUMBER,
                           pData->data.dData, false, "",
                           v8::Local<v8::Object>(), pData->bPersistent == 1);
        pRuntime->PutObjectProperty(ToV8Object(), pData->data.sKey.UTF8Decode(),
                                    pRuntime->NewNumber(pData->data.dData));
        break;
      case JS_GlobalDataType::BOOLEAN:
        SetGlobalVariables(pData->data.sKey, JS_GlobalDataType::BOOLEAN, 0,
                           pData->data.bData == 1, "", v8::Local<v8::Object>(),
                           pData->bPersistent == 1);
        pRuntime->PutObjectProperty(
            ToV8Object(), pData->data.sKey.UTF8Decode(),
            pRuntime->NewBoolean(pData->data.bData == 1));
        break;
      case JS_GlobalDataType::STRING:
        SetGlobalVariables(pData->data.sKey, JS_GlobalDataType::STRING, 0,
                           false, pData->data.sData, v8::Local<v8::Object>(),
                           pData->bPersistent == 1);
        pRuntime->PutObjectProperty(
            ToV8Object(), pData->data.sKey.UTF8Decode(),
            pRuntime->NewString(pData->data.sData.UTF8Decode().AsStringView()));
        break;
      case JS_GlobalDataType::OBJECT: {
        v8::Local<v8::Object> pObj = pRuntime->NewObject();
        if (!pObj.IsEmpty()) {
          PutObjectProperty(pObj, &pData->data);
          SetGlobalVariables(pData->data.sKey, JS_GlobalDataType::OBJECT, 0,
                             false, "", pObj, pData->bPersistent == 1);
          pRuntime->PutObjectProperty(ToV8Object(),
                                      pData->data.sKey.UTF8Decode(), pObj);
        }
      } break;
      case JS_GlobalDataType::NULLOBJ:
        SetGlobalVariables(pData->data.sKey, JS_GlobalDataType::NULLOBJ, 0,
                           false, "", v8::Local<v8::Object>(),
                           pData->bPersistent == 1);
        pRuntime->PutObjectProperty(ToV8Object(), pData->data.sKey.UTF8Decode(),
                                    pRuntime->NewNull());
        break;
    }
  }
}

void CJS_Global::CommitGlobalPersisitentVariables(CJS_Runtime* pRuntime) {
  for (const auto& iter : m_MapGlobal) {
    ByteString name = iter.first;
    JSGlobalData* pData = iter.second.get();
    if (pData->bDeleted) {
      m_pGlobalData->DeleteGlobalVariable(name);
      continue;
    }
    switch (pData->nType) {
      case JS_GlobalDataType::NUMBER:
        m_pGlobalData->SetGlobalVariableNumber(name, pData->dData);
        m_pGlobalData->SetGlobalVariablePersistent(name, pData->bPersistent);
        break;
      case JS_GlobalDataType::BOOLEAN:
        m_pGlobalData->SetGlobalVariableBoolean(name, pData->bData);
        m_pGlobalData->SetGlobalVariablePersistent(name, pData->bPersistent);
        break;
      case JS_GlobalDataType::STRING:
        m_pGlobalData->SetGlobalVariableString(name, pData->sData);
        m_pGlobalData->SetGlobalVariablePersistent(name, pData->bPersistent);
        break;
      case JS_GlobalDataType::OBJECT: {
        CJS_GlobalVariableArray array;
        v8::Local<v8::Object> obj =
            v8::Local<v8::Object>::New(GetIsolate(), pData->pData);
        ObjectToArray(pRuntime, obj, array);
        m_pGlobalData->SetGlobalVariableObject(name, array);
        m_pGlobalData->SetGlobalVariablePersistent(name, pData->bPersistent);
      } break;
      case JS_GlobalDataType::NULLOBJ:
        m_pGlobalData->SetGlobalVariableNull(name);
        m_pGlobalData->SetGlobalVariablePersistent(name, pData->bPersistent);
        break;
    }
  }
}

void CJS_Global::ObjectToArray(CJS_Runtime* pRuntime,
                               v8::Local<v8::Object> pObj,
                               CJS_GlobalVariableArray& array) {
  std::vector<WideString> pKeyList = pRuntime->GetObjectPropertyNames(pObj);
  for (const auto& ws : pKeyList) {
    ByteString sKey = ws.UTF8Encode();
    v8::Local<v8::Value> v = pRuntime->GetObjectProperty(pObj, ws);
    if (v->IsNumber()) {
      CJS_KeyValue* pObjElement = new CJS_KeyValue;
      pObjElement->nType = JS_GlobalDataType::NUMBER;
      pObjElement->sKey = sKey;
      pObjElement->dData = pRuntime->ToDouble(v);
      array.Add(pObjElement);
      continue;
    }
    if (v->IsBoolean()) {
      CJS_KeyValue* pObjElement = new CJS_KeyValue;
      pObjElement->nType = JS_GlobalDataType::BOOLEAN;
      pObjElement->sKey = sKey;
      pObjElement->dData = pRuntime->ToBoolean(v);
      array.Add(pObjElement);
      continue;
    }
    if (v->IsString()) {
      ByteString sValue = ByteString::FromUnicode(pRuntime->ToWideString(v));
      CJS_KeyValue* pObjElement = new CJS_KeyValue;
      pObjElement->nType = JS_GlobalDataType::STRING;
      pObjElement->sKey = sKey;
      pObjElement->sData = sValue;
      array.Add(pObjElement);
      continue;
    }
    if (v->IsObject()) {
      CJS_KeyValue* pObjElement = new CJS_KeyValue;
      pObjElement->nType = JS_GlobalDataType::OBJECT;
      pObjElement->sKey = sKey;
      ObjectToArray(pRuntime, pRuntime->ToObject(v), pObjElement->objData);
      array.Add(pObjElement);
      continue;
    }
    if (v->IsNull()) {
      CJS_KeyValue* pObjElement = new CJS_KeyValue;
      pObjElement->nType = JS_GlobalDataType::NULLOBJ;
      pObjElement->sKey = sKey;
      array.Add(pObjElement);
    }
  }
}

void CJS_Global::PutObjectProperty(v8::Local<v8::Object> pObj,
                                   CJS_KeyValue* pData) {
  CJS_Runtime* pRuntime =
      CJS_Runtime::RuntimeFromIsolateCurrentContext(ToV8Object()->GetIsolate());

  for (int i = 0, sz = pData->objData.Count(); i < sz; i++) {
    CJS_KeyValue* pObjData = pData->objData.GetAt(i);
    switch (pObjData->nType) {
      case JS_GlobalDataType::NUMBER:
        pRuntime->PutObjectProperty(pObj, pObjData->sKey.UTF8Decode(),
                                    pRuntime->NewNumber(pObjData->dData));
        break;
      case JS_GlobalDataType::BOOLEAN:
        pRuntime->PutObjectProperty(pObj, pObjData->sKey.UTF8Decode(),
                                    pRuntime->NewBoolean(pObjData->bData == 1));
        break;
      case JS_GlobalDataType::STRING:
        pRuntime->PutObjectProperty(
            pObj, pObjData->sKey.UTF8Decode(),
            pRuntime->NewString(pObjData->sData.UTF8Decode().AsStringView()));
        break;
      case JS_GlobalDataType::OBJECT: {
        v8::Local<v8::Object> pNewObj = pRuntime->NewObject();
        if (!pNewObj.IsEmpty()) {
          PutObjectProperty(pNewObj, pObjData);
          pRuntime->PutObjectProperty(pObj, pObjData->sKey.UTF8Decode(),
                                      pNewObj);
        }
      } break;
      case JS_GlobalDataType::NULLOBJ:
        pRuntime->PutObjectProperty(pObj, pObjData->sKey.UTF8Decode(),
                                    pRuntime->NewNull());
        break;
    }
  }
}

void CJS_Global::DestroyGlobalPersisitentVariables() {
  m_MapGlobal.clear();
}

CJS_Return CJS_Global::SetGlobalVariables(const ByteString& propname,
                                          JS_GlobalDataType nType,
                                          double dData,
                                          bool bData,
                                          const ByteString& sData,
                                          v8::Local<v8::Object> pData,
                                          bool bDefaultPersistent) {
  if (propname.IsEmpty())
    return CJS_Return(false);

  auto it = m_MapGlobal.find(propname);
  if (it != m_MapGlobal.end()) {
    JSGlobalData* pTemp = it->second.get();
    if (pTemp->bDeleted || pTemp->nType != nType) {
      pTemp->dData = 0;
      pTemp->bData = 0;
      pTemp->sData.clear();
      pTemp->nType = nType;
    }
    pTemp->bDeleted = false;
    switch (nType) {
      case JS_GlobalDataType::NUMBER:
        pTemp->dData = dData;
        break;
      case JS_GlobalDataType::BOOLEAN:
        pTemp->bData = bData;
        break;
      case JS_GlobalDataType::STRING:
        pTemp->sData = sData;
        break;
      case JS_GlobalDataType::OBJECT:
        pTemp->pData.Reset(pData->GetIsolate(), pData);
        break;
      case JS_GlobalDataType::NULLOBJ:
        break;
      default:
        return CJS_Return(false);
    }
    return CJS_Return(true);
  }

  auto pNewData = pdfium::MakeUnique<JSGlobalData>();
  switch (nType) {
    case JS_GlobalDataType::NUMBER:
      pNewData->nType = JS_GlobalDataType::NUMBER;
      pNewData->dData = dData;
      pNewData->bPersistent = bDefaultPersistent;
      break;
    case JS_GlobalDataType::BOOLEAN:
      pNewData->nType = JS_GlobalDataType::BOOLEAN;
      pNewData->bData = bData;
      pNewData->bPersistent = bDefaultPersistent;
      break;
    case JS_GlobalDataType::STRING:
      pNewData->nType = JS_GlobalDataType::STRING;
      pNewData->sData = sData;
      pNewData->bPersistent = bDefaultPersistent;
      break;
    case JS_GlobalDataType::OBJECT:
      pNewData->nType = JS_GlobalDataType::OBJECT;
      pNewData->pData.Reset(pData->GetIsolate(), pData);
      pNewData->bPersistent = bDefaultPersistent;
      break;
    case JS_GlobalDataType::NULLOBJ:
      pNewData->nType = JS_GlobalDataType::NULLOBJ;
      pNewData->bPersistent = bDefaultPersistent;
      break;
    default:
      return CJS_Return(false);
  }
  m_MapGlobal[propname] = std::move(pNewData);
  return CJS_Return(true);
}
