// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CFXJSE_ENGINE_H_
#define FXJS_CFXJSE_ENGINE_H_

#include <map>
#include <memory>
#include <vector>

#include "fxjs/cfx_v8.h"
#include "fxjs/cfxjse_formcalc_context.h"
#include "v8/include/v8.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_script.h"
#include "xfa/fxfa/parser/xfa_resolvenode_rs.h"

#define XFA_RESOLVENODE_TagName 0x0002

class CFXJSE_ResolveProcessor;
class CFXJS_Engine;
class CXFA_List;

class CFXJSE_Engine : public CFX_V8 {
 public:
  static CXFA_Object* ToObject(const v8::FunctionCallbackInfo<v8::Value>& info);
  static CXFA_Object* ToObject(CFXJSE_Value* pValue, CFXJSE_Class* pClass);
  static void GlobalPropertyGetter(CFXJSE_Value* pObject,
                                   const ByteStringView& szPropName,
                                   CFXJSE_Value* pValue);
  static void GlobalPropertySetter(CFXJSE_Value* pObject,
                                   const ByteStringView& szPropName,
                                   CFXJSE_Value* pValue);
  static void NormalPropertyGetter(CFXJSE_Value* pObject,
                                   const ByteStringView& szPropName,
                                   CFXJSE_Value* pValue);
  static void NormalPropertySetter(CFXJSE_Value* pObject,
                                   const ByteStringView& szPropName,
                                   CFXJSE_Value* pValue);
  static CJS_Return NormalMethodCall(
      const v8::FunctionCallbackInfo<v8::Value>& info,
      const WideString& functionName);
  static int32_t NormalPropTypeGetter(CFXJSE_Value* pObject,
                                      const ByteStringView& szPropName,
                                      bool bQueryIn);
  static int32_t GlobalPropTypeGetter(CFXJSE_Value* pObject,
                                      const ByteStringView& szPropName,
                                      bool bQueryIn);

  CFXJSE_Engine(CXFA_Document* pDocument, CFXJS_Engine* fxjs_engine);
  ~CFXJSE_Engine() override;

  void SetEventParam(CXFA_EventParam param) { m_eventParam = param; }
  CXFA_EventParam* GetEventParam() { return &m_eventParam; }
  bool RunScript(CXFA_Script::Type eScriptType,
                 const WideStringView& wsScript,
                 CFXJSE_Value* pRetValue,
                 CXFA_Object* pThisObject);

  bool ResolveObjects(CXFA_Object* refObject,
                      const WideStringView& wsExpression,
                      XFA_RESOLVENODE_RS* resolveNodeRS,
                      uint32_t dwStyles,
                      CXFA_Node* bindNode);
  CFXJSE_Value* GetJSValueFromMap(CXFA_Object* pObject);
  void AddToCacheList(std::unique_ptr<CXFA_List> pList);
  CXFA_Object* GetThisObject() const { return m_pThisObject; }

  int32_t GetIndexByName(CXFA_Node* refNode);
  int32_t GetIndexByClassName(CXFA_Node* refNode);
  WideString GetSomExpression(CXFA_Node* refNode);

  void SetNodesOfRunScript(std::vector<CXFA_Node*>* pArray);
  void AddNodesOfRunScript(CXFA_Node* pNode);
  CFXJSE_Class* GetJseNormalClass();

  void SetRunAtType(XFA_AttributeEnum eRunAt) { m_eRunAtType = eRunAt; }
  bool IsRunAtClient() { return m_eRunAtType != XFA_AttributeEnum::Server; }

  CXFA_Script::Type GetType();
  std::vector<CXFA_Node*>* GetUpObjectArray() { return &m_upObjectArray; }
  CXFA_Document* GetDocument() const { return m_pDocument.Get(); }

  CXFA_Object* ToXFAObject(v8::Local<v8::Value> obj);
  v8::Local<v8::Value> NewXFAObject(CXFA_Object* obj,
                                    v8::Global<v8::FunctionTemplate>& tmpl);

 private:
  CFXJSE_Context* CreateVariablesContext(CXFA_Node* pScriptNode,
                                         CXFA_Node* pSubform);
  void RemoveBuiltInObjs(CFXJSE_Context* pContext) const;
  bool QueryNodeByFlag(CXFA_Node* refNode,
                       const WideStringView& propname,
                       CFXJSE_Value* pValue,
                       uint32_t dwFlag,
                       bool bSetting);
  bool IsStrictScopeInJavaScript();
  CXFA_Object* GetVariablesThis(CXFA_Object* pObject, bool bScriptNode = false);
  bool QueryVariableValue(CXFA_Node* pScriptNode,
                          const ByteStringView& szPropName,
                          CFXJSE_Value* pValue,
                          bool bGetter);
  bool RunVariablesScript(CXFA_Node* pScriptNode);

  UnownedPtr<CXFA_Document> const m_pDocument;
  std::unique_ptr<CFXJSE_Context> m_JsContext;
  CFXJSE_Class* m_pJsClass;
  CXFA_Script::Type m_eScriptType;
  std::map<CXFA_Object*, std::unique_ptr<CFXJSE_Value>> m_mapObjectToValue;
  std::map<CXFA_Object*, std::unique_ptr<CFXJSE_Context>>
      m_mapVariableToContext;
  CXFA_EventParam m_eventParam;
  std::vector<CXFA_Node*> m_upObjectArray;
  // CacheList holds the List items so we can clean them up when we're done.
  std::vector<std::unique_ptr<CXFA_List>> m_CacheList;
  std::vector<CXFA_Node*>* m_pScriptNodeArray;
  std::unique_ptr<CFXJSE_ResolveProcessor> m_ResolveProcessor;
  std::unique_ptr<CFXJSE_FormCalcContext> m_FM2JSContext;
  CXFA_Object* m_pThisObject;
  XFA_AttributeEnum m_eRunAtType;
};

#endif  //  FXJS_CFXJSE_ENGINE_H_
