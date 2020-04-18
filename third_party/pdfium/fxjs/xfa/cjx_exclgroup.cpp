// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_exclgroup.h"

#include <vector>

#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/fxfa.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_exclgroup.h"

const CJX_MethodSpec CJX_ExclGroup::MethodSpecs[] = {
    {"execCalculate", execCalculate_static},
    {"execEvent", execEvent_static},
    {"execInitialize", execInitialize_static},
    {"execValidate", execValidate_static},
    {"selectedMember", selectedMember_static}};

CJX_ExclGroup::CJX_ExclGroup(CXFA_ExclGroup* group) : CJX_Node(group) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_ExclGroup::~CJX_ExclGroup() {}

CJS_Return CJX_ExclGroup::execEvent(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  execSingleEventByName(runtime->ToWideString(params[0]).AsStringView(),
                        XFA_Element::ExclGroup);
  return CJS_Return(true);
}

CJS_Return CJX_ExclGroup::execInitialize(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (pNotify)
    pNotify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Initialize, false,
                                  true);
  return CJS_Return(true);
}

CJS_Return CJX_ExclGroup::execCalculate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (pNotify)
    pNotify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Calculate, false,
                                  true);
  return CJS_Return(true);
}

CJS_Return CJX_ExclGroup::execValidate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* notify = GetDocument()->GetNotify();
  if (!notify)
    return CJS_Return(runtime->NewBoolean(false));

  int32_t iRet = notify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Validate,
                                              false, true);
  return CJS_Return(runtime->NewBoolean(iRet != XFA_EVENTERROR_Error));
}

CJS_Return CJX_ExclGroup::selectedMember(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(runtime->NewNull());

  CXFA_Node* pReturnNode = nullptr;
  if (params.empty()) {
    pReturnNode = node->GetSelectedMember();
  } else {
    pReturnNode = node->SetSelectedMember(
        runtime->ToWideString(params[0]).AsStringView(), true);
  }
  if (!pReturnNode)
    return CJS_Return(runtime->NewNull());

  CFXJSE_Value* value =
      GetDocument()->GetScriptContext()->GetJSValueFromMap(pReturnNode);
  if (!value)
    return CJS_Return(runtime->NewNull());

  return CJS_Return(value->DirectGetValue().Get(runtime->GetIsolate()));
}

void CJX_ExclGroup::defaultValue(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return;

  if (bSetting) {
    node->SetSelectedMemberByValue(pValue->ToWideString().AsStringView(), true,
                                   true, true);
    return;
  }

  WideString wsValue = GetContent(true);
  XFA_VERSION curVersion = GetDocument()->GetCurVersionMode();
  if (wsValue.IsEmpty() && curVersion >= XFA_VERSION_300) {
    pValue->SetNull();
    return;
  }
  pValue->SetString(wsValue.UTF8Encode().AsStringView());
}

void CJX_ExclGroup::rawValue(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  defaultValue(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::transient(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {}

void CJX_ExclGroup::access(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::accessKey(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::anchorType(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::borderColor(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  Script_Som_BorderColor(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::borderWidth(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  Script_Som_BorderWidth(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::colSpan(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::fillColor(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Som_FillColor(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::h(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::hAlign(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::layout(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::mandatory(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Som_Mandatory(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::mandatoryMessage(CFXJSE_Value* pValue,
                                     bool bSetting,
                                     XFA_Attribute eAttribute) {
  Script_Som_MandatoryMessage(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::maxH(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::maxW(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::minH(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::minW(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::presence(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::relevant(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::use(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::usehref(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::validationMessage(CFXJSE_Value* pValue,
                                      bool bSetting,
                                      XFA_Attribute eAttribute) {
  Script_Som_ValidationMessage(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::vAlign(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::w(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::x(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_ExclGroup::y(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
