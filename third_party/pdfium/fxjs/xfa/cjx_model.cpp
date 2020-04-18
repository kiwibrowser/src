// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_model.h"

#include <vector>

#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/parser/cxfa_delta.h"
#include "xfa/fxfa/parser/cxfa_document.h"

const CJX_MethodSpec CJX_Model::MethodSpecs[] = {
    {"clearErrorList", clearErrorList_static},
    {"createNode", createNode_static},
    {"isCompatibleNS", isCompatibleNS_static}};

CJX_Model::CJX_Model(CXFA_Node* node) : CJX_Node(node) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Model::~CJX_Model() {}

CJS_Return CJX_Model::clearErrorList(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_Model::createNode(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString name;
  if (params.size() > 1)
    name = runtime->ToWideString(params[1]);

  WideString nameSpace;
  if (params.size() == 3)
    nameSpace = runtime->ToWideString(params[2]);

  WideString tagName = runtime->ToWideString(params[0]);
  XFA_Element eType = CXFA_Node::NameToElement(tagName);
  CXFA_Node* pNewNode = GetXFANode()->CreateSamePacketNode(eType);
  if (!pNewNode)
    return CJS_Return(runtime->NewNull());

  if (!name.IsEmpty()) {
    if (!pNewNode->HasAttribute(XFA_Attribute::Name))
      return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

    pNewNode->JSObject()->SetAttribute(XFA_Attribute::Name, name.AsStringView(),
                                       true);
    if (pNewNode->GetPacketType() == XFA_PacketType::Datasets)
      pNewNode->CreateXMLMappingNode();
  }

  CFXJSE_Value* value =
      GetDocument()->GetScriptContext()->GetJSValueFromMap(pNewNode);
  if (!value)
    return CJS_Return(runtime->NewNull());

  return CJS_Return(value->DirectGetValue().Get(runtime->GetIsolate()));
}

CJS_Return CJX_Model::isCompatibleNS(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString nameSpace;
  if (params.size() >= 1)
    nameSpace = runtime->ToWideString(params[0]);

  return CJS_Return(
      runtime->NewBoolean(TryNamespace().value_or(WideString()) == nameSpace));
}

void CJX_Model::context(CFXJSE_Value* pValue,
                        bool bSetting,
                        XFA_Attribute eAttribute) {}

void CJX_Model::aliasNode(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {}
