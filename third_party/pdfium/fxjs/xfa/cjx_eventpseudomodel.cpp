// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_eventpseudomodel.h"

#include <vector>

#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/cxfa_ffwidgethandler.h"
#include "xfa/fxfa/parser/cscript_eventpseudomodel.h"

namespace {

void StringProperty(CFXJSE_Value* pValue, WideString* wsValue, bool bSetting) {
  if (bSetting) {
    *wsValue = pValue->ToWideString();
    return;
  }
  pValue->SetString(wsValue->UTF8Encode().AsStringView());
}

void InterProperty(CFXJSE_Value* pValue, int32_t* iValue, bool bSetting) {
  if (bSetting) {
    *iValue = pValue->ToInteger();
    return;
  }
  pValue->SetInteger(*iValue);
}

void BooleanProperty(CFXJSE_Value* pValue, bool* bValue, bool bSetting) {
  if (bSetting) {
    *bValue = pValue->ToBoolean();
    return;
  }
  pValue->SetBoolean(*bValue);
}

}  // namespace

const CJX_MethodSpec CJX_EventPseudoModel::MethodSpecs[] = {
    {"emit", emit_static},
    {"reset", reset_static}};

CJX_EventPseudoModel::CJX_EventPseudoModel(CScript_EventPseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_EventPseudoModel::~CJX_EventPseudoModel() {}

void CJX_EventPseudoModel::change(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Change, bSetting);
}

void CJX_EventPseudoModel::commitKey(CFXJSE_Value* pValue,
                                     bool bSetting,
                                     XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::CommitKey, bSetting);
}

void CJX_EventPseudoModel::fullText(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::FullText, bSetting);
}

void CJX_EventPseudoModel::keyDown(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Keydown, bSetting);
}

void CJX_EventPseudoModel::modifier(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Modifier, bSetting);
}

void CJX_EventPseudoModel::newContentType(CFXJSE_Value* pValue,
                                          bool bSetting,
                                          XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::NewContentType, bSetting);
}

void CJX_EventPseudoModel::newText(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::NewText, bSetting);
}

void CJX_EventPseudoModel::prevContentType(CFXJSE_Value* pValue,
                                           bool bSetting,
                                           XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::PreviousContentType, bSetting);
}

void CJX_EventPseudoModel::prevText(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::PreviousText, bSetting);
}

void CJX_EventPseudoModel::reenter(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Reenter, bSetting);
}

void CJX_EventPseudoModel::selEnd(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::SelectionEnd, bSetting);
}

void CJX_EventPseudoModel::selStart(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::SelectionStart, bSetting);
}

void CJX_EventPseudoModel::shift(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Shift, bSetting);
}

void CJX_EventPseudoModel::soapFaultCode(CFXJSE_Value* pValue,
                                         bool bSetting,
                                         XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::SoapFaultCode, bSetting);
}

void CJX_EventPseudoModel::soapFaultString(CFXJSE_Value* pValue,
                                           bool bSetting,
                                           XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::SoapFaultString, bSetting);
}

void CJX_EventPseudoModel::target(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Property(pValue, XFA_Event::Target, bSetting);
}

CJS_Return CJX_EventPseudoModel::emit(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
  if (!pScriptContext)
    return CJS_Return(true);

  CXFA_EventParam* pEventParam = pScriptContext->GetEventParam();
  if (!pEventParam)
    return CJS_Return(true);

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_FFWidgetHandler* pWidgetHandler = pNotify->GetWidgetHandler();
  if (!pWidgetHandler)
    return CJS_Return(true);

  pWidgetHandler->ProcessEvent(pEventParam->m_pTarget, pEventParam);
  return CJS_Return(true);
}

CJS_Return CJX_EventPseudoModel::reset(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
  if (!pScriptContext)
    return CJS_Return(true);

  CXFA_EventParam* pEventParam = pScriptContext->GetEventParam();
  if (!pEventParam)
    return CJS_Return(true);

  pEventParam->Reset();
  return CJS_Return(true);
}

void CJX_EventPseudoModel::Property(CFXJSE_Value* pValue,
                                    XFA_Event dwFlag,
                                    bool bSetting) {
  CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
  if (!pScriptContext)
    return;

  CXFA_EventParam* pEventParam = pScriptContext->GetEventParam();
  if (!pEventParam)
    return;

  switch (dwFlag) {
    case XFA_Event::CancelAction:
      BooleanProperty(pValue, &pEventParam->m_bCancelAction, bSetting);
      break;
    case XFA_Event::Change:
      StringProperty(pValue, &pEventParam->m_wsChange, bSetting);
      break;
    case XFA_Event::CommitKey:
      InterProperty(pValue, &pEventParam->m_iCommitKey, bSetting);
      break;
    case XFA_Event::FullText:
      StringProperty(pValue, &pEventParam->m_wsFullText, bSetting);
      break;
    case XFA_Event::Keydown:
      BooleanProperty(pValue, &pEventParam->m_bKeyDown, bSetting);
      break;
    case XFA_Event::Modifier:
      BooleanProperty(pValue, &pEventParam->m_bModifier, bSetting);
      break;
    case XFA_Event::NewContentType:
      StringProperty(pValue, &pEventParam->m_wsNewContentType, bSetting);
      break;
    case XFA_Event::NewText:
      StringProperty(pValue, &pEventParam->m_wsNewText, bSetting);
      break;
    case XFA_Event::PreviousContentType:
      StringProperty(pValue, &pEventParam->m_wsPrevContentType, bSetting);
      break;
    case XFA_Event::PreviousText:
      StringProperty(pValue, &pEventParam->m_wsPrevText, bSetting);
      break;
    case XFA_Event::Reenter:
      BooleanProperty(pValue, &pEventParam->m_bReenter, bSetting);
      break;
    case XFA_Event::SelectionEnd:
      InterProperty(pValue, &pEventParam->m_iSelEnd, bSetting);
      break;
    case XFA_Event::SelectionStart:
      InterProperty(pValue, &pEventParam->m_iSelStart, bSetting);
      break;
    case XFA_Event::Shift:
      BooleanProperty(pValue, &pEventParam->m_bShift, bSetting);
      break;
    case XFA_Event::SoapFaultCode:
      StringProperty(pValue, &pEventParam->m_wsSoapFaultCode, bSetting);
      break;
    case XFA_Event::SoapFaultString:
      StringProperty(pValue, &pEventParam->m_wsSoapFaultString, bSetting);
      break;
    case XFA_Event::Target:
    default:
      break;
  }
}
