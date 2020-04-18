// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjs_engine.h"

#include <memory>
#include <utility>
#include <vector>

#include "fxjs/cfxjse_runtimedata.h"
#include "fxjs/cjs_object.h"

class CFXJS_PerObjectData;

// Keep this consistent with the values defined in gin/public/context_holder.h
// (without actually requiring a dependency on gin itself for the standalone
// embedders of PDFIum). The value we want to use is:
//   kPerContextDataStartIndex + kEmbedderPDFium, which is 3.
static const unsigned int kPerContextDataIndex = 3u;
static unsigned int g_embedderDataSlot = 1u;
static v8::Isolate* g_isolate = nullptr;
static size_t g_isolate_ref_count = 0;
static CFX_V8ArrayBufferAllocator* g_arrayBufferAllocator = nullptr;
static v8::Global<v8::ObjectTemplate>* g_DefaultGlobalObjectTemplate = nullptr;
static wchar_t kPerObjectDataTag[] = L"CFXJS_PerObjectData";

// Global weak map to save dynamic objects.
class V8TemplateMapTraits
    : public v8::StdMapTraits<CFXJS_PerObjectData*, v8::Object> {
 public:
  using WeakCallbackDataType = CFXJS_PerObjectData;
  using MapType = v8::
      GlobalValueMap<WeakCallbackDataType*, v8::Object, V8TemplateMapTraits>;

  static const v8::PersistentContainerCallbackType kCallbackType =
      v8::kWeakWithInternalFields;

  static WeakCallbackDataType* WeakCallbackParameter(
      MapType* map,
      WeakCallbackDataType* key,
      v8::Local<v8::Object> value) {
    return key;
  }
  static MapType* MapFromWeakCallbackInfo(
      const v8::WeakCallbackInfo<WeakCallbackDataType>&);
  static WeakCallbackDataType* KeyFromWeakCallbackInfo(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
    return data.GetParameter();
  }
  static void OnWeakCallback(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {}
  static void DisposeWeak(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data);
  static void Dispose(v8::Isolate* isolate,
                      v8::Global<v8::Object> value,
                      WeakCallbackDataType* key);
  static void DisposeCallbackData(WeakCallbackDataType* callbackData) {}
};

class V8TemplateMap {
 public:
  using WeakCallbackDataType = CFXJS_PerObjectData;
  using MapType = v8::
      GlobalValueMap<WeakCallbackDataType*, v8::Object, V8TemplateMapTraits>;

  explicit V8TemplateMap(v8::Isolate* isolate) : m_map(isolate) {}
  ~V8TemplateMap() = default;

  void SetAndMakeWeak(WeakCallbackDataType* key, v8::Local<v8::Object> handle) {
    ASSERT(!m_map.Contains(key));

    // Inserting an object into a GlobalValueMap with the appropriate traits
    // has the side-effect of making the object weak deep in the guts of V8,
    // and arranges for it to be cleaned up by the methods in the traits.
    m_map.Set(key, handle);
  }

  friend class V8TemplateMapTraits;

 private:
  MapType m_map;
};

class CFXJS_PerObjectData {
 public:
  explicit CFXJS_PerObjectData(int nObjDefID) : m_ObjDefID(nObjDefID) {}

  ~CFXJS_PerObjectData() = default;

  static void SetInObject(CFXJS_PerObjectData* pData,
                          v8::Local<v8::Object> pObj) {
    if (pObj->InternalFieldCount() == 2) {
      pObj->SetAlignedPointerInInternalField(
          0, static_cast<void*>(kPerObjectDataTag));
      pObj->SetAlignedPointerInInternalField(1, pData);
    }
  }

  static CFXJS_PerObjectData* GetFromObject(v8::Local<v8::Object> pObj) {
    if (pObj.IsEmpty() || pObj->InternalFieldCount() != 2 ||
        pObj->GetAlignedPointerFromInternalField(0) !=
            static_cast<void*>(kPerObjectDataTag)) {
      return nullptr;
    }
    return static_cast<CFXJS_PerObjectData*>(
        pObj->GetAlignedPointerFromInternalField(1));
  }

  const int m_ObjDefID;
  std::unique_ptr<CJS_Object> m_pPrivate;
};

class CFXJS_ObjDefinition {
 public:
  static int MaxID(v8::Isolate* pIsolate) {
    return FXJS_PerIsolateData::Get(pIsolate)->m_ObjectDefnArray.size();
  }

  static CFXJS_ObjDefinition* ForID(v8::Isolate* pIsolate, int id) {
    // Note: GetAt() halts if out-of-range even in release builds.
    return FXJS_PerIsolateData::Get(pIsolate)->m_ObjectDefnArray[id].get();
  }

  CFXJS_ObjDefinition(v8::Isolate* isolate,
                      const char* sObjName,
                      FXJSOBJTYPE eObjType,
                      CFXJS_Engine::Constructor pConstructor,
                      CFXJS_Engine::Destructor pDestructor)
      : m_ObjName(sObjName),
        m_ObjType(eObjType),
        m_pConstructor(pConstructor),
        m_pDestructor(pDestructor),
        m_pIsolate(isolate) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::FunctionTemplate> fun = v8::FunctionTemplate::New(isolate);
    fun->InstanceTemplate()->SetInternalFieldCount(2);
    fun->SetCallHandler([](const v8::FunctionCallbackInfo<v8::Value>& info) {
      v8::Local<v8::Object> holder = info.Holder();
      ASSERT(holder->InternalFieldCount() == 2);
      holder->SetAlignedPointerInInternalField(0, nullptr);
      holder->SetAlignedPointerInInternalField(1, nullptr);
    });
    if (eObjType == FXJSOBJTYPE_GLOBAL) {
      fun->InstanceTemplate()->Set(
          v8::Symbol::GetToStringTag(isolate),
          v8::String::NewFromUtf8(isolate, "global", v8::NewStringType::kNormal)
              .ToLocalChecked());
    }
    m_FunctionTemplate.Reset(isolate, fun);

    v8::Local<v8::Signature> sig = v8::Signature::New(isolate, fun);
    m_Signature.Reset(isolate, sig);
  }

  int AssignID() {
    FXJS_PerIsolateData* pData = FXJS_PerIsolateData::Get(m_pIsolate);
    pData->m_ObjectDefnArray.emplace_back(this);
    return pData->m_ObjectDefnArray.size() - 1;
  }

  v8::Local<v8::ObjectTemplate> GetInstanceTemplate() {
    v8::EscapableHandleScope scope(m_pIsolate);
    v8::Local<v8::FunctionTemplate> function =
        m_FunctionTemplate.Get(m_pIsolate);
    return scope.Escape(function->InstanceTemplate());
  }

  v8::Local<v8::Signature> GetSignature() {
    v8::EscapableHandleScope scope(m_pIsolate);
    return scope.Escape(m_Signature.Get(m_pIsolate));
  }

  const char* const m_ObjName;
  const FXJSOBJTYPE m_ObjType;
  const CFXJS_Engine::Constructor m_pConstructor;
  const CFXJS_Engine::Destructor m_pDestructor;

  v8::Isolate* m_pIsolate;
  v8::Global<v8::FunctionTemplate> m_FunctionTemplate;
  v8::Global<v8::Signature> m_Signature;
};

static v8::Local<v8::ObjectTemplate> GetGlobalObjectTemplate(
    v8::Isolate* pIsolate) {
  int maxID = CFXJS_ObjDefinition::MaxID(pIsolate);
  for (int i = 0; i < maxID; ++i) {
    CFXJS_ObjDefinition* pObjDef = CFXJS_ObjDefinition::ForID(pIsolate, i);
    if (pObjDef->m_ObjType == FXJSOBJTYPE_GLOBAL)
      return pObjDef->GetInstanceTemplate();
  }
  if (!g_DefaultGlobalObjectTemplate) {
    v8::Local<v8::ObjectTemplate> hGlobalTemplate =
        v8::ObjectTemplate::New(pIsolate);
    hGlobalTemplate->Set(
        v8::Symbol::GetToStringTag(pIsolate),
        v8::String::NewFromUtf8(pIsolate, "global", v8::NewStringType::kNormal)
            .ToLocalChecked());
    g_DefaultGlobalObjectTemplate =
        new v8::Global<v8::ObjectTemplate>(pIsolate, hGlobalTemplate);
  }
  return g_DefaultGlobalObjectTemplate->Get(pIsolate);
}

void V8TemplateMapTraits::Dispose(v8::Isolate* isolate,
                                  v8::Global<v8::Object> value,
                                  WeakCallbackDataType* key) {
  v8::Local<v8::Object> obj = value.Get(isolate);
  if (obj.IsEmpty())
    return;
  int id = CFXJS_Engine::GetObjDefnID(obj);
  if (id == -1)
    return;
  CFXJS_ObjDefinition* pObjDef = CFXJS_ObjDefinition::ForID(isolate, id);
  if (!pObjDef)
    return;
  if (pObjDef->m_pDestructor)
    pObjDef->m_pDestructor(obj);
  CFXJS_Engine::FreeObjectPrivate(obj);
}

void V8TemplateMapTraits::DisposeWeak(
    const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
  // TODO(tsepez): this is expected be called during GC.
}

V8TemplateMapTraits::MapType* V8TemplateMapTraits::MapFromWeakCallbackInfo(
    const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
  V8TemplateMap* pMap =
      FXJS_PerIsolateData::Get(data.GetIsolate())->m_pDynamicObjsMap.get();
  return pMap ? &pMap->m_map : nullptr;
}

void FXJS_Initialize(unsigned int embedderDataSlot, v8::Isolate* pIsolate) {
  if (g_isolate) {
    ASSERT(g_embedderDataSlot == embedderDataSlot);
    ASSERT(g_isolate == pIsolate);
    return;
  }
  g_embedderDataSlot = embedderDataSlot;
  g_isolate = pIsolate;
}

void FXJS_Release() {
  ASSERT(!g_isolate || g_isolate_ref_count == 0);
  delete g_DefaultGlobalObjectTemplate;
  g_DefaultGlobalObjectTemplate = nullptr;
  g_isolate = nullptr;

  delete g_arrayBufferAllocator;
  g_arrayBufferAllocator = nullptr;
}

bool FXJS_GetIsolate(v8::Isolate** pResultIsolate) {
  if (g_isolate) {
    *pResultIsolate = g_isolate;
    return false;
  }
  // Provide backwards compatibility when no external isolate.
  if (!g_arrayBufferAllocator)
    g_arrayBufferAllocator = new CFX_V8ArrayBufferAllocator();
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator = g_arrayBufferAllocator;
  *pResultIsolate = v8::Isolate::New(params);
  return true;
}

size_t FXJS_GlobalIsolateRefCount() {
  return g_isolate_ref_count;
}

FXJS_PerIsolateData::~FXJS_PerIsolateData() {}

// static
void FXJS_PerIsolateData::SetUp(v8::Isolate* pIsolate) {
  if (!pIsolate->GetData(g_embedderDataSlot))
    pIsolate->SetData(g_embedderDataSlot, new FXJS_PerIsolateData(pIsolate));
}

// static
FXJS_PerIsolateData* FXJS_PerIsolateData::Get(v8::Isolate* pIsolate) {
  return static_cast<FXJS_PerIsolateData*>(
      pIsolate->GetData(g_embedderDataSlot));
}

FXJS_PerIsolateData::FXJS_PerIsolateData(v8::Isolate* pIsolate)
    : m_pDynamicObjsMap(new V8TemplateMap(pIsolate)) {}

CFXJS_Engine::CFXJS_Engine() : CFX_V8(nullptr) {}

CFXJS_Engine::CFXJS_Engine(v8::Isolate* pIsolate) : CFX_V8(pIsolate) {}

CFXJS_Engine::~CFXJS_Engine() = default;

// static
CFXJS_Engine* CFXJS_Engine::EngineFromIsolateCurrentContext(
    v8::Isolate* pIsolate) {
  return EngineFromContext(pIsolate->GetCurrentContext());
}

// static
CFXJS_Engine* CFXJS_Engine::EngineFromContext(v8::Local<v8::Context> pContext) {
  return static_cast<CFXJS_Engine*>(
      pContext->GetAlignedPointerFromEmbedderData(kPerContextDataIndex));
}

// static
int CFXJS_Engine::GetObjDefnID(v8::Local<v8::Object> pObj) {
  CFXJS_PerObjectData* pData = CFXJS_PerObjectData::GetFromObject(pObj);
  return pData ? pData->m_ObjDefID : -1;
}

// static
void CFXJS_Engine::SetObjectPrivate(v8::Local<v8::Object> pObj,
                                    std::unique_ptr<CJS_Object> p) {
  CFXJS_PerObjectData* pPerObjectData =
      CFXJS_PerObjectData::GetFromObject(pObj);
  if (!pPerObjectData)
    return;
  pPerObjectData->m_pPrivate = std::move(p);
}

// static
void CFXJS_Engine::FreeObjectPrivate(v8::Local<v8::Object> pObj) {
  CFXJS_PerObjectData* pData = CFXJS_PerObjectData::GetFromObject(pObj);
  pObj->SetAlignedPointerInInternalField(0, nullptr);
  pObj->SetAlignedPointerInInternalField(1, nullptr);
  delete pData;
}

void CFXJS_Engine::SetIntoContext(v8::Local<v8::Context> pContext) {
  pContext->SetAlignedPointerInEmbedderData(kPerContextDataIndex, this);
}

int CFXJS_Engine::DefineObj(const char* sObjName,
                            FXJSOBJTYPE eObjType,
                            CFXJS_Engine::Constructor pConstructor,
                            CFXJS_Engine::Destructor pDestructor) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  FXJS_PerIsolateData::SetUp(GetIsolate());
  CFXJS_ObjDefinition* pObjDef = new CFXJS_ObjDefinition(
      GetIsolate(), sObjName, eObjType, pConstructor, pDestructor);
  return pObjDef->AssignID();
}

void CFXJS_Engine::DefineObjMethod(int nObjDefnID,
                                   const char* sMethodName,
                                   v8::FunctionCallback pMethodCall) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_ObjDefinition* pObjDef =
      CFXJS_ObjDefinition::ForID(GetIsolate(), nObjDefnID);
  v8::Local<v8::FunctionTemplate> fun = v8::FunctionTemplate::New(
      GetIsolate(), pMethodCall, v8::Local<v8::Value>(),
      pObjDef->GetSignature());
  fun->RemovePrototype();
  pObjDef->GetInstanceTemplate()->Set(NewString(sMethodName), fun,
                                      v8::ReadOnly);
}

void CFXJS_Engine::DefineObjProperty(int nObjDefnID,
                                     const char* sPropName,
                                     v8::AccessorGetterCallback pPropGet,
                                     v8::AccessorSetterCallback pPropPut) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_ObjDefinition* pObjDef =
      CFXJS_ObjDefinition::ForID(GetIsolate(), nObjDefnID);
  pObjDef->GetInstanceTemplate()->SetAccessor(NewString(sPropName), pPropGet,
                                              pPropPut);
}

void CFXJS_Engine::DefineObjAllProperties(
    int nObjDefnID,
    v8::GenericNamedPropertyQueryCallback pPropQurey,
    v8::GenericNamedPropertyGetterCallback pPropGet,
    v8::GenericNamedPropertySetterCallback pPropPut,
    v8::GenericNamedPropertyDeleterCallback pPropDel) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_ObjDefinition* pObjDef =
      CFXJS_ObjDefinition::ForID(GetIsolate(), nObjDefnID);
  pObjDef->GetInstanceTemplate()->SetHandler(
      v8::NamedPropertyHandlerConfiguration(
          pPropGet, pPropPut, pPropQurey, pPropDel, nullptr,
          v8::Local<v8::Value>(),
          v8::PropertyHandlerFlags::kOnlyInterceptStrings));
}

void CFXJS_Engine::DefineObjConst(int nObjDefnID,
                                  const char* sConstName,
                                  v8::Local<v8::Value> pDefault) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_ObjDefinition* pObjDef =
      CFXJS_ObjDefinition::ForID(GetIsolate(), nObjDefnID);
  pObjDef->GetInstanceTemplate()->Set(GetIsolate(), sConstName, pDefault);
}

void CFXJS_Engine::DefineGlobalMethod(const char* sMethodName,
                                      v8::FunctionCallback pMethodCall) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::FunctionTemplate> fun =
      v8::FunctionTemplate::New(GetIsolate(), pMethodCall);
  fun->RemovePrototype();
  GetGlobalObjectTemplate(GetIsolate())
      ->Set(NewString(sMethodName), fun, v8::ReadOnly);
}

void CFXJS_Engine::DefineGlobalConst(const wchar_t* sConstName,
                                     v8::FunctionCallback pConstGetter) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::FunctionTemplate> fun =
      v8::FunctionTemplate::New(GetIsolate(), pConstGetter);
  fun->RemovePrototype();
  GetGlobalObjectTemplate(GetIsolate())
      ->SetAccessorProperty(NewString(sConstName), fun);
}

void CFXJS_Engine::InitializeEngine() {
  if (GetIsolate() == g_isolate)
    ++g_isolate_ref_count;

  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());

  // This has to happen before we call GetGlobalObjectTemplate because that
  // method gets the PerIsolateData from GetIsolate().
  FXJS_PerIsolateData::SetUp(GetIsolate());

  v8::Local<v8::Context> v8Context = v8::Context::New(
      GetIsolate(), nullptr, GetGlobalObjectTemplate(GetIsolate()));

  // May not have the internal fields when called from tests.
  v8::Local<v8::Object> pThisProxy = v8Context->Global();
  if (pThisProxy->InternalFieldCount() == 2) {
    pThisProxy->SetAlignedPointerInInternalField(0, nullptr);
    pThisProxy->SetAlignedPointerInInternalField(1, nullptr);
  }
  v8::Local<v8::Object> pThis = pThisProxy->GetPrototype().As<v8::Object>();
  if (pThis->InternalFieldCount() == 2) {
    pThis->SetAlignedPointerInInternalField(0, nullptr);
    pThis->SetAlignedPointerInInternalField(1, nullptr);
  }

  v8::Context::Scope context_scope(v8Context);
  SetIntoContext(v8Context);

  int maxID = CFXJS_ObjDefinition::MaxID(GetIsolate());
  m_StaticObjects.resize(maxID + 1);
  for (int i = 0; i < maxID; ++i) {
    CFXJS_ObjDefinition* pObjDef = CFXJS_ObjDefinition::ForID(GetIsolate(), i);
    if (pObjDef->m_ObjType == FXJSOBJTYPE_GLOBAL) {
      CFXJS_PerObjectData::SetInObject(new CFXJS_PerObjectData(i),
                                       v8Context->Global()
                                           ->GetPrototype()
                                           ->ToObject(v8Context)
                                           .ToLocalChecked());
      if (pObjDef->m_pConstructor) {
        pObjDef->m_pConstructor(this, v8Context->Global()
                                          ->GetPrototype()
                                          ->ToObject(v8Context)
                                          .ToLocalChecked());
      }
    } else if (pObjDef->m_ObjType == FXJSOBJTYPE_STATIC) {
      v8::Local<v8::String> pObjName = NewString(pObjDef->m_ObjName);
      v8::Local<v8::Object> obj = NewFXJSBoundObject(i, true);
      if (!obj.IsEmpty()) {
        v8Context->Global()->Set(v8Context, pObjName, obj).FromJust();
        m_StaticObjects[i] = v8::Global<v8::Object>(GetIsolate(), obj);
      }
    }
  }
  m_V8Context.Reset(GetIsolate(), v8Context);
}

void CFXJS_Engine::ReleaseEngine() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> context = GetV8Context();
  v8::Context::Scope context_scope(context);
  FXJS_PerIsolateData* pData = FXJS_PerIsolateData::Get(GetIsolate());
  if (!pData)
    return;

  m_ConstArrays.clear();

  int maxID = CFXJS_ObjDefinition::MaxID(GetIsolate());
  for (int i = 0; i < maxID; ++i) {
    CFXJS_ObjDefinition* pObjDef = CFXJS_ObjDefinition::ForID(GetIsolate(), i);
    v8::Local<v8::Object> pObj;
    if (pObjDef->m_ObjType == FXJSOBJTYPE_GLOBAL) {
      pObj =
          context->Global()->GetPrototype()->ToObject(context).ToLocalChecked();
    } else if (!m_StaticObjects[i].IsEmpty()) {
      pObj = v8::Local<v8::Object>::New(GetIsolate(), m_StaticObjects[i]);
      m_StaticObjects[i].Reset();
    }

    if (!pObj.IsEmpty()) {
      if (pObjDef->m_pDestructor)
        pObjDef->m_pDestructor(pObj);
      FreeObjectPrivate(pObj);
    }
  }

  m_V8Context.Reset();

  if (GetIsolate() == g_isolate && --g_isolate_ref_count > 0)
    return;

  delete pData;
  GetIsolate()->SetData(g_embedderDataSlot, nullptr);
}

Optional<IJS_Runtime::JS_Error> CFXJS_Engine::Execute(
    const WideString& script) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::TryCatch try_catch(GetIsolate());
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  v8::Local<v8::Script> compiled_script;
  if (!v8::Script::Compile(context, NewString(script.AsStringView()))
           .ToLocal(&compiled_script)) {
    v8::String::Utf8Value error(GetIsolate(), try_catch.Exception());
    v8::Local<v8::Message> msg = try_catch.Message();
    v8::Maybe<int> line = msg->GetLineNumber(context);

    return IJS_Runtime::JS_Error(line.FromMaybe(-1), msg->GetStartColumn(),
                                 WideString::FromUTF8(*error));
  }

  v8::Local<v8::Value> result;
  if (!compiled_script->Run(context).ToLocal(&result)) {
    v8::String::Utf8Value error(GetIsolate(), try_catch.Exception());
    auto msg = try_catch.Message();
    auto line = msg->GetLineNumber(context);
    return IJS_Runtime::JS_Error(line.FromMaybe(-1), msg->GetStartColumn(),
                                 WideString::FromUTF8(*error));
  }
  return pdfium::nullopt;
}

v8::Local<v8::Object> CFXJS_Engine::NewFXJSBoundObject(int nObjDefnID,
                                                       bool bStatic) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  FXJS_PerIsolateData* pData = FXJS_PerIsolateData::Get(GetIsolate());
  if (!pData)
    return v8::Local<v8::Object>();

  if (nObjDefnID < 0 || nObjDefnID >= CFXJS_ObjDefinition::MaxID(GetIsolate()))
    return v8::Local<v8::Object>();

  CFXJS_ObjDefinition* pObjDef =
      CFXJS_ObjDefinition::ForID(GetIsolate(), nObjDefnID);
  v8::Local<v8::Object> obj;
  if (!pObjDef->GetInstanceTemplate()->NewInstance(context).ToLocal(&obj))
    return v8::Local<v8::Object>();

  CFXJS_PerObjectData* pObjData = new CFXJS_PerObjectData(nObjDefnID);
  CFXJS_PerObjectData::SetInObject(pObjData, obj);
  if (pObjDef->m_pConstructor)
    pObjDef->m_pConstructor(this, obj);

  if (!bStatic) {
    auto* pIsolateData = FXJS_PerIsolateData::Get(GetIsolate());
    if (pIsolateData->m_pDynamicObjsMap)
      pIsolateData->m_pDynamicObjsMap->SetAndMakeWeak(pObjData, obj);
  }
  return obj;
}

v8::Local<v8::Object> CFXJS_Engine::GetThisObj() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  if (!FXJS_PerIsolateData::Get(GetIsolate()))
    return v8::Local<v8::Object>();

  // Return the global object.
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  return context->Global()->GetPrototype()->ToObject(context).ToLocalChecked();
}

void CFXJS_Engine::Error(const WideString& message) {
  GetIsolate()->ThrowException(NewString(message.AsStringView()));
}

CJS_Object* CFXJS_Engine::GetObjectPrivate(v8::Local<v8::Object> pObj) {
  CFXJS_PerObjectData* pData = CFXJS_PerObjectData::GetFromObject(pObj);
  if (!pData && !pObj.IsEmpty()) {
    // It could be a global proxy object.
    v8::Local<v8::Value> v = pObj->GetPrototype();
    v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
    if (v->IsObject()) {
      pData = CFXJS_PerObjectData::GetFromObject(
          v->ToObject(context).ToLocalChecked());
    }
  }
  return pData ? pData->m_pPrivate.get() : nullptr;
}

v8::Local<v8::Array> CFXJS_Engine::GetConstArray(const WideString& name) {
  return v8::Local<v8::Array>::New(GetIsolate(), m_ConstArrays[name]);
}

void CFXJS_Engine::SetConstArray(const WideString& name,
                                 v8::Local<v8::Array> array) {
  m_ConstArrays[name] = v8::Global<v8::Array>(GetIsolate(), array);
}
