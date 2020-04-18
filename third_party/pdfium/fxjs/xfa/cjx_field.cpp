// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_field.h"

#include <vector>

#include "core/fxcrt/cfx_decimal.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/fxfa.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_field.h"
#include "xfa/fxfa/parser/cxfa_value.h"

const CJX_MethodSpec CJX_Field::MethodSpecs[] = {
    {"addItem", addItem_static},
    {"boundItem", boundItem_static},
    {"clearItems", clearItems_static},
    {"deleteItem", deleteItem_static},
    {"execCalculate", execCalculate_static},
    {"execEvent", execEvent_static},
    {"execInitialize", execInitialize_static},
    {"execValidate", execValidate_static},
    {"getDisplayItem", getDisplayItem_static},
    {"getItemState", getItemState_static},
    {"getSaveItem", getSaveItem_static},
    {"setItemState", setItemState_static}};

CJX_Field::CJX_Field(CXFA_Field* field) : CJX_Container(field) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Field::~CJX_Field() {}

CJS_Return CJX_Field::clearItems(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_Node* node = GetXFANode();
  if (node->IsWidgetReady())
    node->DeleteItem(-1, true, false);
  return CJS_Return(true);
}

CJS_Return CJX_Field::execEvent(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString eventString = runtime->ToWideString(params[0]);
  int32_t iRet =
      execSingleEventByName(eventString.AsStringView(), XFA_Element::Field);
  if (eventString != L"validate")
    return CJS_Return(true);

  return CJS_Return(runtime->NewBoolean(iRet != XFA_EVENTERROR_Error));
}

CJS_Return CJX_Field::execInitialize(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (pNotify) {
    pNotify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Initialize, false,
                                  false);
  }
  return CJS_Return(true);
}

CJS_Return CJX_Field::deleteItem(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(true);

  bool bValue = node->DeleteItem(runtime->ToInt32(params[0]), true, true);
  return CJS_Return(runtime->NewBoolean(bValue));
}

CJS_Return CJX_Field::getSaveItem(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  int32_t iIndex = runtime->ToInt32(params[0]);
  if (iIndex < 0)
    return CJS_Return(runtime->NewNull());

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(runtime->NewNull());

  Optional<WideString> value = node->GetChoiceListItem(iIndex, true);
  if (!value)
    return CJS_Return(runtime->NewNull());

  return CJS_Return(runtime->NewString(value->UTF8Encode().AsStringView()));
}

CJS_Return CJX_Field::boundItem(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(true);

  WideString value = runtime->ToWideString(params[0]);
  WideString boundValue = node->GetItemValue(value.AsStringView());
  return CJS_Return(runtime->NewString(boundValue.UTF8Encode().AsStringView()));
}

CJS_Return CJX_Field::getItemState(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(true);

  int32_t state = node->GetItemState(runtime->ToInt32(params[0]));
  return CJS_Return(runtime->NewBoolean(state != 0));
}

CJS_Return CJX_Field::execCalculate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (pNotify) {
    pNotify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Calculate, false,
                                  false);
  }
  return CJS_Return(true);
}

CJS_Return CJX_Field::getDisplayItem(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  int32_t iIndex = runtime->ToInt32(params[0]);
  if (iIndex < 0)
    return CJS_Return(runtime->NewNull());

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(runtime->NewNull());

  Optional<WideString> value = node->GetChoiceListItem(iIndex, false);
  if (!value)
    return CJS_Return(runtime->NewNull());

  return CJS_Return(runtime->NewString(value->UTF8Encode().AsStringView()));
}

CJS_Return CJX_Field::setItemState(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(true);

  int32_t iIndex = runtime->ToInt32(params[0]);
  if (runtime->ToInt32(params[1]) != 0) {
    node->SetItemState(iIndex, true, true, true, true);
    return CJS_Return(true);
  }
  if (node->GetItemState(iIndex))
    node->SetItemState(iIndex, false, true, true, true);

  return CJS_Return(true);
}

CJS_Return CJX_Field::addItem(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1 && params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return CJS_Return(true);

  WideString label;
  if (params.size() >= 1)
    label = runtime->ToWideString(params[0]);

  WideString value;
  if (params.size() >= 2)
    value = runtime->ToWideString(params[1]);

  node->InsertItem(label, value, true);
  return CJS_Return(true);
}

CJS_Return CJX_Field::execValidate(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(runtime->NewBoolean(false));

  int32_t iRet = pNotify->ExecEventByDeepFirst(GetXFANode(), XFA_EVENT_Validate,
                                               false, false);
  return CJS_Return(runtime->NewBoolean(iRet != XFA_EVENTERROR_Error));
}

void CJX_Field::defaultValue(CFXJSE_Value* pValue,
                             bool bSetting,
                             XFA_Attribute eAttribute) {
  CXFA_Node* xfaNode = GetXFANode();
  if (!xfaNode->IsWidgetReady())
    return;

  if (bSetting) {
    if (pValue) {
      xfaNode->SetPreNull(xfaNode->IsNull());
      xfaNode->SetIsNull(pValue->IsNull());
    }

    WideString wsNewText;
    if (pValue && !(pValue->IsNull() || pValue->IsUndefined()))
      wsNewText = pValue->ToWideString();
    if (xfaNode->GetUIChildNode()->GetElementType() == XFA_Element::NumericEdit)
      wsNewText = xfaNode->NumericLimit(wsNewText);

    CXFA_Node* pContainerNode = xfaNode->GetContainerNode();
    WideString wsFormatText(wsNewText);
    if (pContainerNode)
      wsFormatText = pContainerNode->GetFormatDataValue(wsNewText);

    SetContent(wsNewText, wsFormatText, true, true, true);
    return;
  }

  WideString content = GetContent(true);
  if (content.IsEmpty()) {
    pValue->SetNull();
    return;
  }

  CXFA_Node* formValue = xfaNode->GetFormValueIfExists();
  CXFA_Node* pNode = formValue ? formValue->GetFirstChild() : nullptr;
  if (pNode && pNode->GetElementType() == XFA_Element::Decimal) {
    if (xfaNode->GetUIChildNode()->GetElementType() ==
            XFA_Element::NumericEdit &&
        (pNode->JSObject()->GetInteger(XFA_Attribute::FracDigits) == -1)) {
      pValue->SetString(content.UTF8Encode().AsStringView());
    } else {
      CFX_Decimal decimal(content.AsStringView());
      pValue->SetFloat((float)(double)decimal);
    }
  } else if (pNode && pNode->GetElementType() == XFA_Element::Integer) {
    pValue->SetInteger(FXSYS_wtoi(content.c_str()));
  } else if (pNode && pNode->GetElementType() == XFA_Element::Boolean) {
    pValue->SetBoolean(FXSYS_wtoi(content.c_str()) == 0 ? false : true);
  } else if (pNode && pNode->GetElementType() == XFA_Element::Float) {
    CFX_Decimal decimal(content.AsStringView());
    pValue->SetFloat((float)(double)decimal);
  } else {
    pValue->SetString(content.UTF8Encode().AsStringView());
  }
}

void CJX_Field::editValue(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return;

  if (bSetting) {
    node->SetValue(XFA_VALUEPICTURE_Edit, pValue->ToWideString());
    return;
  }
  pValue->SetString(
      node->GetValue(XFA_VALUEPICTURE_Edit).UTF8Encode().AsStringView());
}

void CJX_Field::formatMessage(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  Script_Som_Message(pValue, bSetting, XFA_SOM_FormatMessage);
}

void CJX_Field::formattedValue(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return;

  if (bSetting) {
    node->SetValue(XFA_VALUEPICTURE_Display, pValue->ToWideString());
    return;
  }
  pValue->SetString(
      node->GetValue(XFA_VALUEPICTURE_Display).UTF8Encode().AsStringView());
}

void CJX_Field::parentSubform(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->SetNull();
}

void CJX_Field::selectedIndex(CFXJSE_Value* pValue,
                              bool bSetting,
                              XFA_Attribute eAttribute) {
  CXFA_Node* node = GetXFANode();
  if (!node->IsWidgetReady())
    return;

  if (!bSetting) {
    pValue->SetInteger(node->GetSelectedItem(0));
    return;
  }

  int32_t iIndex = pValue->ToInteger();
  if (iIndex == -1) {
    node->ClearAllSelections();
    return;
  }

  node->SetItemState(iIndex, true, true, true, true);
}

void CJX_Field::access(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::accessKey(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::anchorType(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::borderColor(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Som_BorderColor(pValue, bSetting, eAttribute);
}

void CJX_Field::borderWidth(CFXJSE_Value* pValue,
                            bool bSetting,
                            XFA_Attribute eAttribute) {
  Script_Som_BorderWidth(pValue, bSetting, eAttribute);
}

void CJX_Field::colSpan(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::fillColor(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Som_FillColor(pValue, bSetting, eAttribute);
}

void CJX_Field::fontColor(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Som_FontColor(pValue, bSetting, eAttribute);
}

void CJX_Field::h(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::hAlign(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::locale(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::mandatory(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  Script_Som_Mandatory(pValue, bSetting, eAttribute);
}

void CJX_Field::mandatoryMessage(CFXJSE_Value* pValue,
                                 bool bSetting,
                                 XFA_Attribute eAttribute) {
  Script_Som_MandatoryMessage(pValue, bSetting, eAttribute);
}

void CJX_Field::maxH(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::maxW(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::minH(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::minW(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::presence(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::rawValue(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  defaultValue(pValue, bSetting, eAttribute);
}

void CJX_Field::relevant(CFXJSE_Value* pValue,
                         bool bSetting,
                         XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::rotate(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::use(CFXJSE_Value* pValue,
                    bool bSetting,
                    XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::usehref(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::validationMessage(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  Script_Som_ValidationMessage(pValue, bSetting, eAttribute);
}

void CJX_Field::vAlign(CFXJSE_Value* pValue,
                       bool bSetting,
                       XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::w(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::x(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Field::y(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}
