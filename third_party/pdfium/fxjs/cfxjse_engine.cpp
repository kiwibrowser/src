// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjse_engine.h"

#include <utility>

#include "core/fxcrt/autorestorer.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_extension.h"
#include "fxjs/cfxjs_engine.h"
#include "fxjs/cfxjse_class.h"
#include "fxjs/cfxjse_resolveprocessor.h"
#include "fxjs/cfxjse_value.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffdoc.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_localemgr.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_nodehelper.h"
#include "xfa/fxfa/parser/cxfa_object.h"
#include "xfa/fxfa/parser/cxfa_thisproxy.h"
#include "xfa/fxfa/parser/cxfa_treelist.h"
#include "xfa/fxfa/parser/xfa_basic_data.h"
#include "xfa/fxfa/parser/xfa_resolvenode_rs.h"
#include "xfa/fxfa/parser/xfa_utils.h"

const FXJSE_CLASS_DESCRIPTOR GlobalClassDescriptor = {
    "Root",   // name
    nullptr,  // methods
    0,        // method count
    CFXJSE_Engine::GlobalPropTypeGetter,
    CFXJSE_Engine::GlobalPropertyGetter,
    CFXJSE_Engine::GlobalPropertySetter,
    CFXJSE_Engine::NormalMethodCall,
};

const FXJSE_CLASS_DESCRIPTOR NormalClassDescriptor = {
    "XFAObject",  // name
    nullptr,      // methods
    0,            // method count
    CFXJSE_Engine::NormalPropTypeGetter,
    CFXJSE_Engine::NormalPropertyGetter,
    CFXJSE_Engine::NormalPropertySetter,
    CFXJSE_Engine::NormalMethodCall,
};

const FXJSE_CLASS_DESCRIPTOR VariablesClassDescriptor = {
    "XFAScriptObject",  // name
    nullptr,            // methods
    0,                  // method count
    CFXJSE_Engine::NormalPropTypeGetter,
    CFXJSE_Engine::GlobalPropertyGetter,
    CFXJSE_Engine::GlobalPropertySetter,
    CFXJSE_Engine::NormalMethodCall,
};

namespace {

const char kFormCalcRuntime[] = "pfm_rt";

CXFA_ThisProxy* ToThisProxy(CFXJSE_Value* pValue, CFXJSE_Class* pClass) {
  return static_cast<CXFA_ThisProxy*>(pValue->ToHostObject(pClass));
}

}  // namespace

// static
CXFA_Object* CFXJSE_Engine::ToObject(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (!info.Holder()->IsObject())
    return nullptr;

  CFXJSE_HostObject* hostObj =
      FXJSE_RetrieveObjectBinding(info.Holder().As<v8::Object>(), nullptr);
  if (!hostObj || hostObj->type() != CFXJSE_HostObject::kXFA)
    return nullptr;
  return static_cast<CXFA_Object*>(hostObj);
}

// static.
CXFA_Object* CFXJSE_Engine::ToObject(CFXJSE_Value* pValue,
                                     CFXJSE_Class* pClass) {
  CFXJSE_HostObject* pHostObj = pValue->ToHostObject(pClass);
  if (!pHostObj || pHostObj->type() != CFXJSE_HostObject::kXFA)
    return nullptr;
  return static_cast<CXFA_Object*>(pHostObj);
}

CFXJSE_Engine::CFXJSE_Engine(CXFA_Document* pDocument,
                             CFXJS_Engine* fxjs_engine)
    : CFX_V8(fxjs_engine->GetIsolate()),
      m_pDocument(pDocument),
      m_JsContext(CFXJSE_Context::Create(fxjs_engine->GetIsolate(),
                                         fxjs_engine,
                                         &GlobalClassDescriptor,
                                         pDocument->GetRoot())),
      m_pJsClass(nullptr),
      m_eScriptType(CXFA_Script::Type::Unknown),
      m_pScriptNodeArray(nullptr),
      m_ResolveProcessor(pdfium::MakeUnique<CFXJSE_ResolveProcessor>()),
      m_pThisObject(nullptr),
      m_eRunAtType(XFA_AttributeEnum::Client) {
  RemoveBuiltInObjs(m_JsContext.get());
  m_JsContext->EnableCompatibleMode();

  // Don't know if this can happen before we remove the builtin objs and set
  // compatibility mode.
  m_pJsClass =
      CFXJSE_Class::Create(m_JsContext.get(), &NormalClassDescriptor, false);
}

CFXJSE_Engine::~CFXJSE_Engine() {
  for (const auto& pair : m_mapVariableToContext)
    delete ToThisProxy(pair.second->GetGlobalObject().get(), nullptr);
}

bool CFXJSE_Engine::RunScript(CXFA_Script::Type eScriptType,
                              const WideStringView& wsScript,
                              CFXJSE_Value* hRetValue,
                              CXFA_Object* pThisObject) {
  ByteString btScript;
  AutoRestorer<CXFA_Script::Type> typeRestorer(&m_eScriptType);
  m_eScriptType = eScriptType;
  if (eScriptType == CXFA_Script::Type::Formcalc) {
    if (!m_FM2JSContext) {
      m_FM2JSContext = pdfium::MakeUnique<CFXJSE_FormCalcContext>(
          GetIsolate(), m_JsContext.get(), m_pDocument.Get());
    }
    CFX_WideTextBuf wsJavaScript;
    if (!CFXJSE_FormCalcContext::Translate(wsScript, &wsJavaScript)) {
      hRetValue->SetUndefined();
      return false;
    }
    btScript = FX_UTF8Encode(wsJavaScript.AsStringView());
  } else {
    btScript = FX_UTF8Encode(wsScript);
  }
  AutoRestorer<CXFA_Object*> nodeRestorer(&m_pThisObject);
  m_pThisObject = pThisObject;
  CFXJSE_Value* pValue = pThisObject ? GetJSValueFromMap(pThisObject) : nullptr;
  return m_JsContext->ExecuteScript(btScript.c_str(), hRetValue, pValue);
}

bool CFXJSE_Engine::QueryNodeByFlag(CXFA_Node* refNode,
                                    const WideStringView& propname,
                                    CFXJSE_Value* pValue,
                                    uint32_t dwFlag,
                                    bool bSetting) {
  if (!refNode)
    return false;

  XFA_RESOLVENODE_RS resolveRs;
  if (!ResolveObjects(refNode, propname, &resolveRs, dwFlag, nullptr))
    return false;
  if (resolveRs.dwFlags == XFA_ResolveNode_RSType_Nodes) {
    pValue->Assign(GetJSValueFromMap(resolveRs.objects.front()));
    return true;
  }
  if (resolveRs.dwFlags == XFA_ResolveNode_RSType_Attribute) {
    const XFA_SCRIPTATTRIBUTEINFO* lpAttributeInfo = resolveRs.pScriptAttribute;
    if (lpAttributeInfo) {
      CJX_Object* jsObject = resolveRs.objects.front()->JSObject();
      (jsObject->*(lpAttributeInfo->callback))(pValue, bSetting,
                                               lpAttributeInfo->attribute);
    }
  }
  return true;
}

void CFXJSE_Engine::GlobalPropertySetter(CFXJSE_Value* pObject,
                                         const ByteStringView& szPropName,
                                         CFXJSE_Value* pValue) {
  CXFA_Object* lpOrginalNode = ToObject(pObject, nullptr);
  CXFA_Document* pDoc = lpOrginalNode->GetDocument();
  CFXJSE_Engine* lpScriptContext = pDoc->GetScriptContext();
  CXFA_Node* pRefNode = ToNode(lpScriptContext->GetThisObject());
  if (lpOrginalNode->IsVariablesThis())
    pRefNode = ToNode(lpScriptContext->GetVariablesThis(lpOrginalNode));

  WideString wsPropName = WideString::FromUTF8(szPropName);
  if (lpScriptContext->QueryNodeByFlag(
          pRefNode, wsPropName.AsStringView(), pValue,
          XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings |
              XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Properties |
              XFA_RESOLVENODE_Attributes,
          true)) {
    return;
  }
  if (lpOrginalNode->IsVariablesThis()) {
    if (pValue && pValue->IsUndefined()) {
      pObject->SetObjectOwnProperty(szPropName, pValue);
      return;
    }
  }
  CXFA_FFNotify* pNotify = pDoc->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  hDoc->GetDocEnvironment()->SetPropertyInNonXFAGlobalObject(hDoc, szPropName,
                                                             pValue);
}

void CFXJSE_Engine::GlobalPropertyGetter(CFXJSE_Value* pObject,
                                         const ByteStringView& szPropName,
                                         CFXJSE_Value* pValue) {
  CXFA_Object* pOriginalObject = ToObject(pObject, nullptr);
  CXFA_Document* pDoc = pOriginalObject->GetDocument();
  CFXJSE_Engine* lpScriptContext = pDoc->GetScriptContext();
  WideString wsPropName = WideString::FromUTF8(szPropName);
  if (lpScriptContext->GetType() == CXFA_Script::Type::Formcalc) {
    if (szPropName == kFormCalcRuntime) {
      lpScriptContext->m_FM2JSContext->GlobalPropertyGetter(pValue);
      return;
    }
    XFA_HashCode uHashCode = static_cast<XFA_HashCode>(
        FX_HashCode_GetW(wsPropName.AsStringView(), false));
    if (uHashCode != XFA_HASHCODE_Layout) {
      CXFA_Object* pObj =
          lpScriptContext->GetDocument()->GetXFAObject(uHashCode);
      if (pObj) {
        pValue->Assign(lpScriptContext->GetJSValueFromMap(pObj));
        return;
      }
    }
  }

  CXFA_Node* pRefNode = ToNode(lpScriptContext->GetThisObject());
  if (pOriginalObject->IsVariablesThis())
    pRefNode = ToNode(lpScriptContext->GetVariablesThis(pOriginalObject));

  if (lpScriptContext->QueryNodeByFlag(
          pRefNode, wsPropName.AsStringView(), pValue,
          XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Properties |
              XFA_RESOLVENODE_Attributes,
          false)) {
    return;
  }

  if (lpScriptContext->QueryNodeByFlag(
          pRefNode, wsPropName.AsStringView(), pValue,
          XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings, false)) {
    return;
  }

  CXFA_Object* pScriptObject =
      lpScriptContext->GetVariablesThis(pOriginalObject, true);
  if (pScriptObject && lpScriptContext->QueryVariableValue(
                           pScriptObject->AsNode(), szPropName, pValue, true)) {
    return;
  }

  CXFA_FFNotify* pNotify = pDoc->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  hDoc->GetDocEnvironment()->GetPropertyFromNonXFAGlobalObject(hDoc, szPropName,
                                                               pValue);
}

int32_t CFXJSE_Engine::GlobalPropTypeGetter(CFXJSE_Value* pOriginalValue,
                                            const ByteStringView& szPropName,
                                            bool bQueryIn) {
  CXFA_Object* pObject = ToObject(pOriginalValue, nullptr);
  if (!pObject)
    return FXJSE_ClassPropType_None;

  CFXJSE_Engine* lpScriptContext = pObject->GetDocument()->GetScriptContext();
  pObject = lpScriptContext->GetVariablesThis(pObject);
  WideString wsPropName = WideString::FromUTF8(szPropName);
  if (pObject->JSObject()->HasMethod(wsPropName))
    return FXJSE_ClassPropType_Method;

  return FXJSE_ClassPropType_Property;
}

void CFXJSE_Engine::NormalPropertyGetter(CFXJSE_Value* pOriginalValue,
                                         const ByteStringView& szPropName,
                                         CFXJSE_Value* pReturnValue) {
  CXFA_Object* pOriginalObject = ToObject(pOriginalValue, nullptr);
  if (!pOriginalObject) {
    pReturnValue->SetUndefined();
    return;
  }

  WideString wsPropName = WideString::FromUTF8(szPropName);
  CFXJSE_Engine* lpScriptContext =
      pOriginalObject->GetDocument()->GetScriptContext();
  CXFA_Object* pObject = lpScriptContext->GetVariablesThis(pOriginalObject);
  if (wsPropName == L"xfa") {
    CFXJSE_Value* pValue = lpScriptContext->GetJSValueFromMap(
        lpScriptContext->GetDocument()->GetRoot());
    pReturnValue->Assign(pValue);
    return;
  }

  bool bRet = lpScriptContext->QueryNodeByFlag(
      ToNode(pObject), wsPropName.AsStringView(), pReturnValue,
      XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Properties |
          XFA_RESOLVENODE_Attributes,
      false);
  if (bRet)
    return;

  if (pObject == lpScriptContext->GetThisObject() ||
      (lpScriptContext->GetType() == CXFA_Script::Type::Javascript &&
       !lpScriptContext->IsStrictScopeInJavaScript())) {
    bRet = lpScriptContext->QueryNodeByFlag(
        ToNode(pObject), wsPropName.AsStringView(), pReturnValue,
        XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings, false);
  }
  if (bRet)
    return;

  CXFA_Object* pScriptObject =
      lpScriptContext->GetVariablesThis(pOriginalObject, true);
  if (pScriptObject) {
    bRet = lpScriptContext->QueryVariableValue(ToNode(pScriptObject),
                                               szPropName, pReturnValue, true);
  }
  if (!bRet)
    pReturnValue->SetUndefined();
}

void CFXJSE_Engine::NormalPropertySetter(CFXJSE_Value* pOriginalValue,
                                         const ByteStringView& szPropName,
                                         CFXJSE_Value* pReturnValue) {
  CXFA_Object* pOriginalObject = ToObject(pOriginalValue, nullptr);
  if (!pOriginalObject)
    return;

  CFXJSE_Engine* lpScriptContext =
      pOriginalObject->GetDocument()->GetScriptContext();
  CXFA_Object* pObject = lpScriptContext->GetVariablesThis(pOriginalObject);
  WideString wsPropName = WideString::FromUTF8(szPropName);
  const XFA_SCRIPTATTRIBUTEINFO* lpAttributeInfo = XFA_GetScriptAttributeByName(
      pObject->GetElementType(), wsPropName.AsStringView());
  if (lpAttributeInfo) {
    CJX_Object* jsObject = pObject->JSObject();
    (jsObject->*(lpAttributeInfo->callback))(pReturnValue, true,
                                             lpAttributeInfo->attribute);
    return;
  }

  if (pObject->IsNode()) {
    if (wsPropName[0] == '#')
      wsPropName = wsPropName.Right(wsPropName.GetLength() - 1);

    CXFA_Node* pNode = ToNode(pObject);
    CXFA_Node* pPropOrChild = nullptr;
    XFA_Element eType = CXFA_Node::NameToElement(wsPropName);
    if (eType != XFA_Element::Unknown) {
      pPropOrChild =
          pNode->JSObject()->GetOrCreateProperty<CXFA_Node>(0, eType);
    } else {
      pPropOrChild = pNode->GetFirstChildByName(wsPropName.AsStringView());
    }

    if (pPropOrChild) {
      const XFA_SCRIPTATTRIBUTEINFO* lpAttrInfo = XFA_GetScriptAttributeByName(
          pPropOrChild->GetElementType(), L"{default}");
      if (lpAttrInfo) {
        pPropOrChild->JSObject()->Script_Som_DefaultValue(
            pReturnValue, true, XFA_Attribute::Unknown);
        return;
      }
    }
  }

  CXFA_Object* pScriptObject =
      lpScriptContext->GetVariablesThis(pOriginalObject, true);
  if (pScriptObject) {
    lpScriptContext->QueryVariableValue(ToNode(pScriptObject), szPropName,
                                        pReturnValue, false);
  }
}

int32_t CFXJSE_Engine::NormalPropTypeGetter(CFXJSE_Value* pOriginalValue,
                                            const ByteStringView& szPropName,
                                            bool bQueryIn) {
  CXFA_Object* pObject = ToObject(pOriginalValue, nullptr);
  if (!pObject)
    return FXJSE_ClassPropType_None;

  CFXJSE_Engine* lpScriptContext = pObject->GetDocument()->GetScriptContext();
  pObject = lpScriptContext->GetVariablesThis(pObject);
  XFA_Element eType = pObject->GetElementType();
  WideString wsPropName = WideString::FromUTF8(szPropName);
  if (pObject->JSObject()->HasMethod(wsPropName))
    return FXJSE_ClassPropType_Method;

  if (bQueryIn &&
      !XFA_GetScriptAttributeByName(eType, wsPropName.AsStringView())) {
    return FXJSE_ClassPropType_None;
  }
  return FXJSE_ClassPropType_Property;
}

CJS_Return CFXJSE_Engine::NormalMethodCall(
    const v8::FunctionCallbackInfo<v8::Value>& info,
    const WideString& functionName) {
  CXFA_Object* pObject = ToObject(info);
  if (!pObject)
    return CJS_Return(false);

  CFXJSE_Engine* lpScriptContext = pObject->GetDocument()->GetScriptContext();
  pObject = lpScriptContext->GetVariablesThis(pObject);

  std::vector<v8::Local<v8::Value>> parameters;
  for (unsigned int i = 0; i < (unsigned int)info.Length(); i++)
    parameters.push_back(info[i]);

  return pObject->JSObject()->RunMethod(functionName, parameters);
}

bool CFXJSE_Engine::IsStrictScopeInJavaScript() {
  return m_pDocument->HasFlag(XFA_DOCFLAG_StrictScoping);
}

CXFA_Script::Type CFXJSE_Engine::GetType() {
  return m_eScriptType;
}

CFXJSE_Context* CFXJSE_Engine::CreateVariablesContext(CXFA_Node* pScriptNode,
                                                      CXFA_Node* pSubform) {
  if (!pScriptNode || !pSubform)
    return nullptr;

  auto pNewContext =
      CFXJSE_Context::Create(GetIsolate(), nullptr, &VariablesClassDescriptor,
                             new CXFA_ThisProxy(pSubform, pScriptNode));
  RemoveBuiltInObjs(pNewContext.get());
  pNewContext->EnableCompatibleMode();
  CFXJSE_Context* pResult = pNewContext.get();
  m_mapVariableToContext[pScriptNode] = std::move(pNewContext);
  return pResult;
}

CXFA_Object* CFXJSE_Engine::GetVariablesThis(CXFA_Object* pObject,
                                             bool bScriptNode) {
  if (!pObject->IsVariablesThis())
    return pObject;

  CXFA_ThisProxy* pProxy = static_cast<CXFA_ThisProxy*>(pObject);
  return bScriptNode ? pProxy->GetScriptNode() : pProxy->GetThisNode();
}

bool CFXJSE_Engine::RunVariablesScript(CXFA_Node* pScriptNode) {
  if (!pScriptNode)
    return false;

  if (pScriptNode->GetElementType() != XFA_Element::Script)
    return true;

  CXFA_Node* pParent = pScriptNode->GetParent();
  if (!pParent || pParent->GetElementType() != XFA_Element::Variables)
    return false;

  auto it = m_mapVariableToContext.find(pScriptNode);
  if (it != m_mapVariableToContext.end() && it->second)
    return true;

  CXFA_Node* pTextNode = pScriptNode->GetFirstChild();
  if (!pTextNode)
    return false;

  Optional<WideString> wsScript =
      pTextNode->JSObject()->TryCData(XFA_Attribute::Value, true);
  if (!wsScript)
    return false;

  ByteString btScript = wsScript->UTF8Encode();
  auto hRetValue = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
  CXFA_Node* pThisObject = pParent->GetParent();
  CFXJSE_Context* pVariablesContext =
      CreateVariablesContext(pScriptNode, pThisObject);
  AutoRestorer<CXFA_Object*> nodeRestorer(&m_pThisObject);
  m_pThisObject = pThisObject;
  return pVariablesContext->ExecuteScript(btScript.c_str(), hRetValue.get());
}

bool CFXJSE_Engine::QueryVariableValue(CXFA_Node* pScriptNode,
                                       const ByteStringView& szPropName,
                                       CFXJSE_Value* pValue,
                                       bool bGetter) {
  if (!pScriptNode || pScriptNode->GetElementType() != XFA_Element::Script)
    return false;

  CXFA_Node* variablesNode = pScriptNode->GetParent();
  if (!variablesNode ||
      variablesNode->GetElementType() != XFA_Element::Variables)
    return false;

  auto it = m_mapVariableToContext.find(pScriptNode);
  if (it == m_mapVariableToContext.end() || !it->second)
    return false;

  CFXJSE_Context* pVariableContext = it->second.get();
  std::unique_ptr<CFXJSE_Value> pObject = pVariableContext->GetGlobalObject();
  auto hVariableValue = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
  if (!bGetter) {
    pObject->SetObjectOwnProperty(szPropName, pValue);
    return true;
  }
  if (pObject->HasObjectOwnProperty(szPropName, false)) {
    pObject->GetObjectProperty(szPropName, hVariableValue.get());
    if (hVariableValue->IsFunction())
      pValue->SetFunctionBind(hVariableValue.get(), pObject.get());
    else if (bGetter)
      pValue->Assign(hVariableValue.get());
    else
      hVariableValue.get()->Assign(pValue);
    return true;
  }
  return false;
}

void CFXJSE_Engine::RemoveBuiltInObjs(CFXJSE_Context* pContext) const {
  const ByteStringView OBJ_NAME[2] = {"Number", "Date"};
  std::unique_ptr<CFXJSE_Value> pObject = pContext->GetGlobalObject();
  auto hProp = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
  for (int i = 0; i < 2; ++i) {
    if (pObject->GetObjectProperty(OBJ_NAME[i], hProp.get()))
      pObject->DeleteObjectProperty(OBJ_NAME[i]);
  }
}

CFXJSE_Class* CFXJSE_Engine::GetJseNormalClass() {
  return m_pJsClass;
}

bool CFXJSE_Engine::ResolveObjects(CXFA_Object* refObject,
                                   const WideStringView& wsExpression,
                                   XFA_RESOLVENODE_RS* resolveNodeRS,
                                   uint32_t dwStyles,
                                   CXFA_Node* bindNode) {
  if (wsExpression.IsEmpty())
    return false;

  if (m_eScriptType != CXFA_Script::Type::Formcalc ||
      (dwStyles & (XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings))) {
    m_upObjectArray.clear();
  }
  if (refObject && refObject->IsNode() &&
      (dwStyles & (XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings))) {
    m_upObjectArray.push_back(refObject->AsNode());
  }

  bool bNextCreate = false;
  if (dwStyles & XFA_RESOLVENODE_CreateNode)
    m_ResolveProcessor->GetNodeHelper()->SetCreateNodeType(bindNode);

  m_ResolveProcessor->GetNodeHelper()->m_pCreateParent = nullptr;
  m_ResolveProcessor->GetNodeHelper()->m_iCurAllStart = -1;

  CFXJSE_ResolveNodeData rndFind(this);
  int32_t nStart = 0;
  int32_t nLevel = 0;

  std::vector<CXFA_Object*> findObjects;
  findObjects.push_back(refObject ? refObject : m_pDocument->GetRoot());
  int32_t nNodes = 0;
  while (true) {
    nNodes = pdfium::CollectionSize<int32_t>(findObjects);
    int32_t i = 0;
    rndFind.m_dwStyles = dwStyles;
    m_ResolveProcessor->SetCurStart(nStart);
    nStart = m_ResolveProcessor->GetFilter(wsExpression, nStart, rndFind);
    if (nStart < 1) {
      if ((dwStyles & XFA_RESOLVENODE_CreateNode) && !bNextCreate) {
        CXFA_Node* pDataNode = nullptr;
        nStart = m_ResolveProcessor->GetNodeHelper()->m_iCurAllStart;
        if (nStart != -1) {
          pDataNode = m_pDocument->GetNotBindNode(findObjects);
          if (pDataNode) {
            findObjects.clear();
            findObjects.push_back(pDataNode);
            break;
          }
        } else {
          pDataNode = findObjects.front()->AsNode();
          findObjects.clear();
          findObjects.push_back(pDataNode);
          break;
        }
        dwStyles |= XFA_RESOLVENODE_Bind;
        findObjects.clear();
        findObjects.push_back(
            m_ResolveProcessor->GetNodeHelper()->m_pAllStartParent);
        continue;
      }
      break;
    }
    if (bNextCreate) {
      bool bCreate =
          m_ResolveProcessor->GetNodeHelper()->ResolveNodes_CreateNode(
              rndFind.m_wsName, rndFind.m_wsCondition,
              nStart ==
                  pdfium::base::checked_cast<int32_t>(wsExpression.GetLength()),
              this);
      if (bCreate)
        continue;

      break;
    }

    std::vector<CXFA_Object*> retObjects;
    while (i < nNodes) {
      bool bDataBind = false;
      if (((dwStyles & XFA_RESOLVENODE_Bind) ||
           (dwStyles & XFA_RESOLVENODE_CreateNode)) &&
          nNodes > 1) {
        CFXJSE_ResolveNodeData rndBind(nullptr);
        m_ResolveProcessor->GetFilter(wsExpression, nStart, rndBind);
        m_ResolveProcessor->SetIndexDataBind(rndBind.m_wsCondition, i, nNodes);
        bDataBind = true;
      }
      rndFind.m_CurObject = findObjects[i++];
      rndFind.m_nLevel = nLevel;
      rndFind.m_dwFlag = XFA_ResolveNode_RSType_Nodes;
      if (!m_ResolveProcessor->Resolve(rndFind))
        continue;

      if (rndFind.m_dwFlag == XFA_ResolveNode_RSType_Attribute &&
          rndFind.m_pScriptAttribute &&
          nStart <
              pdfium::base::checked_cast<int32_t>(wsExpression.GetLength())) {
        auto pValue = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
        CJX_Object* jsObject = rndFind.m_Objects.front()->JSObject();
        (jsObject->*(rndFind.m_pScriptAttribute->callback))(
            pValue.get(), false, rndFind.m_pScriptAttribute->attribute);
        rndFind.m_Objects.front() = ToObject(pValue.get(), nullptr);
      }
      if (!m_upObjectArray.empty())
        m_upObjectArray.pop_back();
      retObjects.insert(retObjects.end(), rndFind.m_Objects.begin(),
                        rndFind.m_Objects.end());
      rndFind.m_Objects.clear();
      if (bDataBind)
        break;
    }
    findObjects.clear();

    nNodes = pdfium::CollectionSize<int32_t>(retObjects);
    if (nNodes < 1) {
      if (dwStyles & XFA_RESOLVENODE_CreateNode) {
        bNextCreate = true;
        if (!m_ResolveProcessor->GetNodeHelper()->m_pCreateParent) {
          m_ResolveProcessor->GetNodeHelper()->m_pCreateParent =
              ToNode(rndFind.m_CurObject);
          m_ResolveProcessor->GetNodeHelper()->m_iCreateCount = 1;
        }
        bool bCreate =
            m_ResolveProcessor->GetNodeHelper()->ResolveNodes_CreateNode(
                rndFind.m_wsName, rndFind.m_wsCondition,
                nStart == pdfium::base::checked_cast<int32_t>(
                              wsExpression.GetLength()),
                this);
        if (bCreate)
          continue;
      }
      break;
    }

    findObjects =
        std::vector<CXFA_Object*>(retObjects.begin(), retObjects.end());
    rndFind.m_Objects.clear();
    if (nLevel == 0)
      dwStyles &= ~(XFA_RESOLVENODE_Parent | XFA_RESOLVENODE_Siblings);

    nLevel++;
  }

  if (!bNextCreate) {
    resolveNodeRS->dwFlags = rndFind.m_dwFlag;
    if (nNodes > 0) {
      resolveNodeRS->objects.insert(resolveNodeRS->objects.end(),
                                    findObjects.begin(), findObjects.end());
    }
    if (rndFind.m_dwFlag == XFA_ResolveNode_RSType_Attribute) {
      resolveNodeRS->pScriptAttribute = rndFind.m_pScriptAttribute;
      return 1;
    }
  }
  if (dwStyles & (XFA_RESOLVENODE_CreateNode | XFA_RESOLVENODE_Bind |
                  XFA_RESOLVENODE_BindNew)) {
    CXFA_NodeHelper* helper = m_ResolveProcessor->GetNodeHelper();
    if (helper->m_pCreateParent)
      resolveNodeRS->objects.push_back(helper->m_pCreateParent);
    else
      helper->CreateNode_ForCondition(rndFind.m_wsCondition);

    resolveNodeRS->dwFlags = helper->m_iCreateFlag;
    if (resolveNodeRS->dwFlags == XFA_ResolveNode_RSType_CreateNodeOne) {
      if (helper->m_iCurAllStart != -1)
        resolveNodeRS->dwFlags = XFA_ResolveNode_RSType_CreateNodeMidAll;
    }

    if (!bNextCreate && (dwStyles & XFA_RESOLVENODE_CreateNode))
      resolveNodeRS->dwFlags = XFA_ResolveNode_RSType_ExistNodes;

    return !resolveNodeRS->objects.empty();
  }
  return nNodes > 0;
}

void CFXJSE_Engine::AddToCacheList(std::unique_ptr<CXFA_List> pList) {
  m_CacheList.push_back(std::move(pList));
}

CFXJSE_Value* CFXJSE_Engine::GetJSValueFromMap(CXFA_Object* pObject) {
  if (!pObject)
    return nullptr;
  if (pObject->IsNode())
    RunVariablesScript(pObject->AsNode());

  auto iter = m_mapObjectToValue.find(pObject);
  if (iter != m_mapObjectToValue.end())
    return iter->second.get();

  auto jsValue = pdfium::MakeUnique<CFXJSE_Value>(GetIsolate());
  jsValue->SetObject(pObject, m_pJsClass);

  CFXJSE_Value* pValue = jsValue.get();
  m_mapObjectToValue.insert(std::make_pair(pObject, std::move(jsValue)));
  return pValue;
}

int32_t CFXJSE_Engine::GetIndexByName(CXFA_Node* refNode) {
  CXFA_NodeHelper* lpNodeHelper = m_ResolveProcessor->GetNodeHelper();
  return lpNodeHelper->GetIndex(refNode, XFA_LOGIC_Transparent,
                                lpNodeHelper->NodeIsProperty(refNode), false);
}

int32_t CFXJSE_Engine::GetIndexByClassName(CXFA_Node* refNode) {
  CXFA_NodeHelper* lpNodeHelper = m_ResolveProcessor->GetNodeHelper();
  return lpNodeHelper->GetIndex(refNode, XFA_LOGIC_Transparent,
                                lpNodeHelper->NodeIsProperty(refNode), true);
}

WideString CFXJSE_Engine::GetSomExpression(CXFA_Node* refNode) {
  CXFA_NodeHelper* lpNodeHelper = m_ResolveProcessor->GetNodeHelper();
  return lpNodeHelper->GetNameExpression(refNode, true, XFA_LOGIC_Transparent);
}

void CFXJSE_Engine::SetNodesOfRunScript(std::vector<CXFA_Node*>* pArray) {
  m_pScriptNodeArray = pArray;
}

void CFXJSE_Engine::AddNodesOfRunScript(CXFA_Node* pNode) {
  if (m_pScriptNodeArray && !pdfium::ContainsValue(*m_pScriptNodeArray, pNode))
    m_pScriptNodeArray->push_back(pNode);
}

CXFA_Object* CFXJSE_Engine::ToXFAObject(v8::Local<v8::Value> obj) {
  ASSERT(!obj.IsEmpty());

  if (!obj->IsObject())
    return nullptr;

  CFXJSE_HostObject* hostObj =
      FXJSE_RetrieveObjectBinding(obj.As<v8::Object>(), nullptr);
  if (!hostObj || hostObj->type() != CFXJSE_HostObject::kXFA)
    return nullptr;
  return static_cast<CXFA_Object*>(hostObj);
}

v8::Local<v8::Value> CFXJSE_Engine::NewXFAObject(
    CXFA_Object* obj,
    v8::Global<v8::FunctionTemplate>& tmpl) {
  v8::EscapableHandleScope scope(GetIsolate());
  v8::Local<v8::FunctionTemplate> klass =
      v8::Local<v8::FunctionTemplate>::New(GetIsolate(), tmpl);
  v8::Local<v8::Object> object = klass->InstanceTemplate()->NewInstance();
  FXJSE_UpdateObjectBinding(object, obj);
  return scope.Escape(object);
}
