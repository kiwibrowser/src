// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjse_context.h"

#include <utility>

#include "fxjs/cfxjs_engine.h"
#include "fxjs/cfxjse_class.h"
#include "fxjs/cfxjse_value.h"
#include "third_party/base/ptr_util.h"

namespace {

const char szCompatibleModeScript[] =
    "(function(global, list) {\n"
    "  'use strict';\n"
    "  var objname;\n"
    "  for (objname in list) {\n"
    "    var globalobj = global[objname];\n"
    "    if (globalobj) {\n"
    "      list[objname].forEach(function(name) {\n"
    "        if (!globalobj[name]) {\n"
    "          Object.defineProperty(globalobj, name, {\n"
    "            writable: true,\n"
    "            enumerable: false,\n"
    "            value: (function(obj) {\n"
    "              if (arguments.length === 0) {\n"
    "                throw new TypeError('missing argument 0 when calling "
    "                    function ' + objname + '.' + name);\n"
    "              }\n"
    "              return globalobj.prototype[name].apply(obj, "
    "                  Array.prototype.slice.call(arguments, 1));\n"
    "            })\n"
    "          });\n"
    "        }\n"
    "      });\n"
    "    }\n"
    "  }\n"
    "}(this, {String: ['substr', 'toUpperCase']}));";

wchar_t g_FXJSETagString[] = L"FXJSE_HostObject";

v8::Local<v8::Object> CreateReturnValue(v8::Isolate* pIsolate,
                                        v8::TryCatch& trycatch) {
  v8::Local<v8::Object> hReturnValue = v8::Object::New(pIsolate);
  if (trycatch.HasCaught()) {
    v8::Local<v8::Value> hException = trycatch.Exception();
    v8::Local<v8::Message> hMessage = trycatch.Message();
    if (hException->IsObject()) {
      v8::Local<v8::Value> hValue;
      hValue = hException.As<v8::Object>()->Get(
          v8::String::NewFromUtf8(pIsolate, "name"));
      if (hValue->IsString() || hValue->IsStringObject())
        hReturnValue->Set(0, hValue);
      else
        hReturnValue->Set(0, v8::String::NewFromUtf8(pIsolate, "Error"));

      hValue = hException.As<v8::Object>()->Get(
          v8::String::NewFromUtf8(pIsolate, "message"));
      if (hValue->IsString() || hValue->IsStringObject())
        hReturnValue->Set(1, hValue);
      else
        hReturnValue->Set(1, hMessage->Get());
    } else {
      hReturnValue->Set(0, v8::String::NewFromUtf8(pIsolate, "Error"));
      hReturnValue->Set(1, hMessage->Get());
    }
    hReturnValue->Set(2, hException);
    hReturnValue->Set(
        3, v8::Integer::New(
               pIsolate, hMessage->GetLineNumber(pIsolate->GetCurrentContext())
                             .FromMaybe(0)));
    hReturnValue->Set(4, hMessage->GetSourceLine(pIsolate->GetCurrentContext())
                             .FromMaybe(v8::Local<v8::String>()));
    v8::Maybe<int32_t> maybe_int =
        hMessage->GetStartColumn(pIsolate->GetCurrentContext());
    hReturnValue->Set(5, v8::Integer::New(pIsolate, maybe_int.FromMaybe(0)));
    maybe_int = hMessage->GetEndColumn(pIsolate->GetCurrentContext());
    hReturnValue->Set(6, v8::Integer::New(pIsolate, maybe_int.FromMaybe(0)));
  }
  return hReturnValue;
}

v8::Local<v8::Object> GetGlobalObjectFromContext(
    v8::Local<v8::Context> hContext) {
  return hContext->Global()->GetPrototype().As<v8::Object>();
}

}  // namespace

// Note, not in the anonymous namespace due to the friend call
// in cfxjse_context.h
// TODO(dsinclair): Remove the friending, use public methods.
class CFXJSE_ScopeUtil_IsolateHandleContext {
 public:
  explicit CFXJSE_ScopeUtil_IsolateHandleContext(CFXJSE_Context* pContext)
      : m_context(pContext),
        m_parent(pContext->m_pIsolate),
        m_cscope(v8::Local<v8::Context>::New(pContext->m_pIsolate,
                                             pContext->m_hContext)) {}
  v8::Isolate* GetIsolate() { return m_context->m_pIsolate; }
  v8::Local<v8::Context> GetLocalContext() {
    return v8::Local<v8::Context>::New(m_context->m_pIsolate,
                                       m_context->m_hContext);
  }

 private:
  CFXJSE_ScopeUtil_IsolateHandleContext(
      const CFXJSE_ScopeUtil_IsolateHandleContext&) = delete;
  void operator=(const CFXJSE_ScopeUtil_IsolateHandleContext&) = delete;
  void* operator new(size_t size) = delete;
  void operator delete(void*, size_t) = delete;

  UnownedPtr<CFXJSE_Context> m_context;
  CFXJSE_ScopeUtil_IsolateHandle m_parent;
  v8::Context::Scope m_cscope;
};

void FXJSE_UpdateObjectBinding(v8::Local<v8::Object>& hObject,
                               CFXJSE_HostObject* lpNewBinding) {
  ASSERT(!hObject.IsEmpty());
  ASSERT(hObject->InternalFieldCount() == 2);
  hObject->SetAlignedPointerInInternalField(0, g_FXJSETagString);
  hObject->SetAlignedPointerInInternalField(1, lpNewBinding);
}

CFXJSE_HostObject* FXJSE_RetrieveObjectBinding(v8::Local<v8::Object> hJSObject,
                                               CFXJSE_Class* lpClass) {
  ASSERT(!hJSObject.IsEmpty());
  if (!hJSObject->IsObject())
    return nullptr;

  v8::Local<v8::Object> hObject = hJSObject;
  if (hObject->InternalFieldCount() != 2) {
    v8::Local<v8::Value> hProtoObject = hObject->GetPrototype();
    if (hProtoObject.IsEmpty() || !hProtoObject->IsObject())
      return nullptr;

    hObject = hProtoObject.As<v8::Object>();
    if (hObject->InternalFieldCount() != 2)
      return nullptr;
  }
  if (hObject->GetAlignedPointerFromInternalField(0) != g_FXJSETagString)
    return nullptr;
  if (lpClass) {
    v8::Local<v8::FunctionTemplate> hClass =
        v8::Local<v8::FunctionTemplate>::New(
            lpClass->GetContext()->GetIsolate(), lpClass->GetTemplate());
    if (!hClass->HasInstance(hObject))
      return nullptr;
  }
  return static_cast<CFXJSE_HostObject*>(
      hObject->GetAlignedPointerFromInternalField(1));
}

// static
std::unique_ptr<CFXJSE_Context> CFXJSE_Context::Create(
    v8::Isolate* pIsolate,
    CFXJS_Engine* pOptionalEngineToSet,
    const FXJSE_CLASS_DESCRIPTOR* pGlobalClass,
    CFXJSE_HostObject* pGlobalObject) {
  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  auto pContext = pdfium::MakeUnique<CFXJSE_Context>(pIsolate);

  v8::Local<v8::ObjectTemplate> hObjectTemplate;
  if (pGlobalClass) {
    CFXJSE_Class* pGlobalClassObj =
        CFXJSE_Class::Create(pContext.get(), pGlobalClass, true);
    ASSERT(pGlobalClassObj);
    v8::Local<v8::FunctionTemplate> hFunctionTemplate =
        v8::Local<v8::FunctionTemplate>::New(pIsolate,
                                             pGlobalClassObj->m_hTemplate);
    hObjectTemplate = hFunctionTemplate->InstanceTemplate();
  } else {
    hObjectTemplate = v8::ObjectTemplate::New(pIsolate);
    hObjectTemplate->SetInternalFieldCount(2);
  }
  hObjectTemplate->Set(
      v8::Symbol::GetToStringTag(pIsolate),
      v8::String::NewFromUtf8(pIsolate, "global", v8::NewStringType::kNormal)
          .ToLocalChecked());

  v8::Local<v8::Context> hNewContext =
      v8::Context::New(pIsolate, nullptr, hObjectTemplate);

  v8::Local<v8::Object> pThisProxy = hNewContext->Global();
  ASSERT(pThisProxy->InternalFieldCount() == 2);
  pThisProxy->SetAlignedPointerInInternalField(0, nullptr);
  pThisProxy->SetAlignedPointerInInternalField(1, nullptr);

  v8::Local<v8::Object> pThis = pThisProxy->GetPrototype().As<v8::Object>();
  ASSERT(pThis->InternalFieldCount() == 2);
  pThis->SetAlignedPointerInInternalField(0, nullptr);
  pThis->SetAlignedPointerInInternalField(1, nullptr);

  v8::Local<v8::Context> hRootContext = v8::Local<v8::Context>::New(
      pIsolate, CFXJSE_RuntimeData::Get(pIsolate)->m_hRootContext);
  hNewContext->SetSecurityToken(hRootContext->GetSecurityToken());

  v8::Local<v8::Object> hGlobalObject = GetGlobalObjectFromContext(hNewContext);
  FXJSE_UpdateObjectBinding(hGlobalObject, pGlobalObject);
  if (pOptionalEngineToSet)
    pOptionalEngineToSet->SetIntoContext(hNewContext);

  pContext->m_hContext.Reset(pIsolate, hNewContext);
  return pContext;
}

CFXJSE_Context::CFXJSE_Context(v8::Isolate* pIsolate) : m_pIsolate(pIsolate) {}

CFXJSE_Context::~CFXJSE_Context() {}

std::unique_ptr<CFXJSE_Value> CFXJSE_Context::GetGlobalObject() {
  auto pValue = pdfium::MakeUnique<CFXJSE_Value>(m_pIsolate);
  CFXJSE_ScopeUtil_IsolateHandleContext scope(this);
  v8::Local<v8::Context> hContext =
      v8::Local<v8::Context>::New(m_pIsolate, m_hContext);
  v8::Local<v8::Object> hGlobalObject = GetGlobalObjectFromContext(hContext);
  pValue->ForceSetValue(hGlobalObject);
  return pValue;
}

v8::Local<v8::Context> CFXJSE_Context::GetContext() {
  return v8::Local<v8::Context>::New(m_pIsolate, m_hContext);
}

void CFXJSE_Context::AddClass(std::unique_ptr<CFXJSE_Class> pClass) {
  m_rgClasses.push_back(std::move(pClass));
}

CFXJSE_Class* CFXJSE_Context::GetClassByName(
    const ByteStringView& szName) const {
  auto pClass =
      std::find_if(m_rgClasses.begin(), m_rgClasses.end(),
                   [szName](const std::unique_ptr<CFXJSE_Class>& item) {
                     return szName == item->m_szClassName;
                   });
  return pClass != m_rgClasses.end() ? pClass->get() : nullptr;
}

void CFXJSE_Context::EnableCompatibleMode() {
  ExecuteScript(szCompatibleModeScript, nullptr, nullptr);
}

bool CFXJSE_Context::ExecuteScript(const char* szScript,
                                   CFXJSE_Value* lpRetValue,
                                   CFXJSE_Value* lpNewThisObject) {
  CFXJSE_ScopeUtil_IsolateHandleContext scope(this);
  v8::Local<v8::Context> hContext = m_pIsolate->GetCurrentContext();
  v8::TryCatch trycatch(m_pIsolate);
  v8::Local<v8::String> hScriptString =
      v8::String::NewFromUtf8(m_pIsolate, szScript);
  if (!lpNewThisObject) {
    v8::Local<v8::Script> hScript;
    if (v8::Script::Compile(hContext, hScriptString).ToLocal(&hScript)) {
      ASSERT(!trycatch.HasCaught());
      v8::Local<v8::Value> hValue;
      if (hScript->Run(hContext).ToLocal(&hValue)) {
        ASSERT(!trycatch.HasCaught());
        if (lpRetValue)
          lpRetValue->m_hValue.Reset(m_pIsolate, hValue);
        return true;
      }
    }
    if (lpRetValue) {
      lpRetValue->m_hValue.Reset(m_pIsolate,
                                 CreateReturnValue(m_pIsolate, trycatch));
    }
    return false;
  }

  v8::Local<v8::Value> hNewThis =
      v8::Local<v8::Value>::New(m_pIsolate, lpNewThisObject->m_hValue);
  ASSERT(!hNewThis.IsEmpty());
  v8::Local<v8::Script> hWrapper =
      v8::Script::Compile(
          hContext,
          v8::String::NewFromUtf8(
              m_pIsolate, "(function () { return eval(arguments[0]); })"))
          .ToLocalChecked();
  v8::Local<v8::Value> hWrapperValue;
  if (hWrapper->Run(hContext).ToLocal(&hWrapperValue)) {
    ASSERT(!trycatch.HasCaught());
    v8::Local<v8::Function> hWrapperFn = hWrapperValue.As<v8::Function>();
    v8::Local<v8::Value> rgArgs[] = {hScriptString};
    v8::Local<v8::Value> hValue;
    if (hWrapperFn->Call(hContext, hNewThis.As<v8::Object>(), 1, rgArgs)
            .ToLocal(&hValue)) {
      ASSERT(!trycatch.HasCaught());
      if (lpRetValue)
        lpRetValue->m_hValue.Reset(m_pIsolate, hValue);
      return true;
    }
  }
  if (lpRetValue) {
    lpRetValue->m_hValue.Reset(m_pIsolate,
                               CreateReturnValue(m_pIsolate, trycatch));
  }
  return false;
}
