// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_node.h"

#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/fx_codepage.h"
#include "fxjs/cfxjse_engine.h"
#include "fxjs/js_resources.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffdoc.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_document_parser.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/xfa_utils.h"

namespace {

enum class EventAppliesToo {
  kNone = 0,
  kAll = 1,
  kAllNonRecursive = 2,
  kSubform = 3,
  kFieldOrExclusion = 4,
  kField = 5,
  kSignature = 6,
  kChoiceList = 7
};

struct XFA_ExecEventParaInfo {
 public:
  uint32_t m_uHash;
  const wchar_t* m_lpcEventName;
  XFA_EVENTTYPE m_eventType;
  EventAppliesToo m_validFlags;
};

const XFA_ExecEventParaInfo gs_eventParaInfos[] = {
    {0x109d7ce7, L"mouseEnter", XFA_EVENT_MouseEnter, EventAppliesToo::kField},
    {0x1bfc72d9, L"preOpen", XFA_EVENT_PreOpen, EventAppliesToo::kChoiceList},
    {0x2196a452, L"initialize", XFA_EVENT_Initialize, EventAppliesToo::kAll},
    {0x27410f03, L"mouseExit", XFA_EVENT_MouseExit, EventAppliesToo::kField},
    {0x36f1c6d8, L"preSign", XFA_EVENT_PreSign, EventAppliesToo::kSignature},
    {0x4731d6ba, L"exit", XFA_EVENT_Exit, EventAppliesToo::kAllNonRecursive},
    {0x7233018a, L"validate", XFA_EVENT_Validate, EventAppliesToo::kAll},
    {0x8808385e, L"indexChange", XFA_EVENT_IndexChange,
     EventAppliesToo::kSubform},
    {0x891f4606, L"change", XFA_EVENT_Change,
     EventAppliesToo::kFieldOrExclusion},
    {0x9f693b21, L"mouseDown", XFA_EVENT_MouseDown, EventAppliesToo::kField},
    {0xcdce56b3, L"full", XFA_EVENT_Full, EventAppliesToo::kFieldOrExclusion},
    {0xd576d08e, L"mouseUp", XFA_EVENT_MouseUp, EventAppliesToo::kField},
    {0xd95657a6, L"click", XFA_EVENT_Click, EventAppliesToo::kFieldOrExclusion},
    {0xdbfbe02e, L"calculate", XFA_EVENT_Calculate, EventAppliesToo::kAll},
    {0xe25fa7b8, L"postOpen", XFA_EVENT_PostOpen, EventAppliesToo::kChoiceList},
    {0xe28dce7e, L"enter", XFA_EVENT_Enter, EventAppliesToo::kAllNonRecursive},
    {0xfd54fbb7, L"postSign", XFA_EVENT_PostSign, EventAppliesToo::kSignature},
};

const XFA_ExecEventParaInfo* GetEventParaInfoByName(
    WideStringView wsEventName) {
  if (wsEventName.IsEmpty())
    return nullptr;

  uint32_t uHash = FX_HashCode_GetW(wsEventName, false);
  auto* result = std::lower_bound(
      std::begin(gs_eventParaInfos), std::end(gs_eventParaInfos), uHash,
      [](const XFA_ExecEventParaInfo& iter, const uint16_t& hash) {
        return iter.m_uHash < hash;
      });
  if (result != std::end(gs_eventParaInfos) && result->m_uHash == uHash)
    return result;
  return nullptr;
}

}  // namespace

const CJX_MethodSpec CJX_Node::MethodSpecs[] = {
    {"applyXSL", applyXSL_static},
    {"assignNode", assignNode_static},
    {"clone", clone_static},
    {"getAttribute", getAttribute_static},
    {"getElement", getElement_static},
    {"isPropertySpecified", isPropertySpecified_static},
    {"loadXML", loadXML_static},
    {"saveFilteredXML", saveFilteredXML_static},
    {"saveXML", saveXML_static},
    {"setAttribute", setAttribute_static},
    {"setElement", setElement_static}};

CJX_Node::CJX_Node(CXFA_Node* node) : CJX_Tree(node) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_Node::~CJX_Node() = default;

CXFA_Node* CJX_Node::GetXFANode() {
  return static_cast<CXFA_Node*>(GetXFAObject());
}

const CXFA_Node* CJX_Node::GetXFANode() const {
  return static_cast<const CXFA_Node*>(GetXFAObject());
}

CJS_Return CJX_Node::applyXSL(CFX_V8* runtime,
                              const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  // TODO(weili): check whether we need to implement this, pdfium:501.
  return CJS_Return(true);
}

CJS_Return CJX_Node::assignNode(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  // TODO(weili): check whether we need to implement this, pdfium:501.
  return CJS_Return(true);
}

CJS_Return CJX_Node::clone(CFX_V8* runtime,
                           const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* pCloneNode = GetXFANode()->Clone(runtime->ToBoolean(params[0]));
  CFXJSE_Value* value =
      GetDocument()->GetScriptContext()->GetJSValueFromMap(pCloneNode);
  if (!value)
    return CJS_Return(runtime->NewNull());
  return CJS_Return(value->DirectGetValue().Get(runtime->GetIsolate()));
}

CJS_Return CJX_Node::getAttribute(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString expression = runtime->ToWideString(params[0]);
  return CJS_Return(runtime->NewString(
      GetAttribute(expression.AsStringView()).UTF8Encode().AsStringView()));
}

CJS_Return CJX_Node::getElement(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString expression = runtime->ToWideString(params[0]);
  int32_t iValue = params.size() >= 2 ? runtime->ToInt32(params[1]) : 0;

  CXFA_Node* pNode = GetOrCreateProperty<CXFA_Node>(
      iValue, CXFA_Node::NameToElement(expression));
  if (!pNode)
    return CJS_Return(runtime->NewNull());

  CFXJSE_Value* value =
      GetDocument()->GetScriptContext()->GetJSValueFromMap(pNode);
  if (!value)
    return CJS_Return(runtime->NewNull());
  return CJS_Return(value->DirectGetValue().Get(runtime->GetIsolate()));
}

CJS_Return CJX_Node::isPropertySpecified(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString expression = runtime->ToWideString(params[0]);
  XFA_Attribute attr = CXFA_Node::NameToAttribute(expression.AsStringView());
  if (attr != XFA_Attribute::Unknown && HasAttribute(attr))
    return CJS_Return(runtime->NewBoolean(true));

  bool bParent = params.size() < 2 || runtime->ToBoolean(params[1]);
  int32_t iIndex = params.size() == 3 ? runtime->ToInt32(params[2]) : 0;
  XFA_Element eType = CXFA_Node::NameToElement(expression);
  bool bHas = !!GetOrCreateProperty<CXFA_Node>(iIndex, eType);
  if (!bHas && bParent && GetXFANode()->GetParent()) {
    // Also check on the parent.
    auto* jsnode = GetXFANode()->GetParent()->JSObject();
    bHas = jsnode->HasAttribute(attr) ||
           !!jsnode->GetOrCreateProperty<CXFA_Node>(iIndex, eType);
  }
  return CJS_Return(runtime->NewBoolean(bHas));
}

CJS_Return CJX_Node::loadXML(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  ByteString expression = runtime->ToByteString(params[0]);
  if (expression.IsEmpty())
    return CJS_Return(true);

  bool bIgnoreRoot = true;
  if (params.size() >= 2)
    bIgnoreRoot = runtime->ToBoolean(params[1]);

  bool bOverwrite = 0;
  if (params.size() >= 3)
    bOverwrite = runtime->ToBoolean(params[2]);

  auto pParser = pdfium::MakeUnique<CXFA_DocumentParser>(GetDocument());
  if (!pParser)
    return CJS_Return(true);

  CFX_XMLNode* pXMLNode = pParser->ParseXMLData(expression);
  if (!pXMLNode)
    return CJS_Return(true);

  if (bIgnoreRoot &&
      (pXMLNode->GetType() != FX_XMLNODE_Element ||
       XFA_RecognizeRichText(static_cast<CFX_XMLElement*>(pXMLNode)))) {
    bIgnoreRoot = false;
  }

  CXFA_Node* pFakeRoot = GetXFANode()->Clone(false);
  WideString wsContentType = GetCData(XFA_Attribute::ContentType);
  if (!wsContentType.IsEmpty()) {
    pFakeRoot->JSObject()->SetCData(XFA_Attribute::ContentType,
                                    WideString(wsContentType), false, false);
  }

  CFX_XMLNode* pFakeXMLRoot = pFakeRoot->GetXMLMappingNode();
  if (!pFakeXMLRoot) {
    CFX_XMLNode* pThisXMLRoot = GetXFANode()->GetXMLMappingNode();
    CFX_XMLNode* clone;
    if (pThisXMLRoot) {
      clone = pThisXMLRoot->Clone(GetXFANode()
                                      ->GetDocument()
                                      ->GetNotify()
                                      ->GetHDOC()
                                      ->GetXMLDocument());
    } else {
      clone = GetXFANode()
                  ->GetDocument()
                  ->GetNotify()
                  ->GetHDOC()
                  ->GetXMLDocument()
                  ->CreateNode<CFX_XMLElement>(
                      WideString(GetXFANode()->GetClassName()));
    }
    pFakeXMLRoot = clone;
  }

  if (bIgnoreRoot) {
    CFX_XMLNode* pXMLChild = pXMLNode->GetFirstChild();
    while (pXMLChild) {
      CFX_XMLNode* pXMLSibling = pXMLChild->GetNextSibling();
      pXMLNode->RemoveChildNode(pXMLChild);
      pFakeXMLRoot->AppendChild(pXMLChild);
      pXMLChild = pXMLSibling;
    }
  } else {
    CFX_XMLNode* pXMLParent = pXMLNode->GetParent();
    if (pXMLParent)
      pXMLParent->RemoveChildNode(pXMLNode);

    pFakeXMLRoot->AppendChild(pXMLNode);
  }

  pParser->ConstructXFANode(pFakeRoot, pFakeXMLRoot);
  pFakeRoot = pParser->GetRootNode();
  if (!pFakeRoot)
    return CJS_Return(true);

  if (bOverwrite) {
    CXFA_Node* pChild = GetXFANode()->GetFirstChild();
    CXFA_Node* pNewChild = pFakeRoot->GetFirstChild();
    int32_t index = 0;
    while (pNewChild) {
      CXFA_Node* pItem = pNewChild->GetNextSibling();
      pFakeRoot->RemoveChild(pNewChild, true);
      GetXFANode()->InsertChild(index++, pNewChild);
      pNewChild->SetFlagAndNotify(XFA_NodeFlag_Initialized);
      pNewChild = pItem;
    }

    while (pChild) {
      CXFA_Node* pItem = pChild->GetNextSibling();
      GetXFANode()->RemoveChild(pChild, true);
      pFakeRoot->InsertChild(pChild, nullptr);
      pChild = pItem;
    }

    if (GetXFANode()->GetPacketType() == XFA_PacketType::Form &&
        GetXFANode()->GetElementType() == XFA_Element::ExData) {
      CFX_XMLNode* pTempXMLNode = GetXFANode()->GetXMLMappingNode();
      GetXFANode()->SetXMLMappingNode(pFakeXMLRoot);

      if (pTempXMLNode && !pTempXMLNode->GetParent())
        pFakeXMLRoot = pTempXMLNode;
      else
        pFakeXMLRoot = nullptr;
    }
    MoveBufferMapData(pFakeRoot, GetXFANode());
  } else {
    CXFA_Node* pChild = pFakeRoot->GetFirstChild();
    while (pChild) {
      CXFA_Node* pItem = pChild->GetNextSibling();
      pFakeRoot->RemoveChild(pChild, true);
      GetXFANode()->InsertChild(pChild, nullptr);
      pChild->SetFlagAndNotify(XFA_NodeFlag_Initialized);
      pChild = pItem;
    }
  }

  if (pFakeXMLRoot) {
    pFakeRoot->SetXMLMappingNode(std::move(pFakeXMLRoot));
  }
  pFakeRoot->SetFlag(XFA_NodeFlag_HasRemovedChildren);

  return CJS_Return(true);
}

CJS_Return CJX_Node::saveFilteredXML(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // TODO(weili): Check whether we need to implement this, pdfium:501.
  return CJS_Return(true);
}

CJS_Return CJX_Node::saveXML(CFX_V8* runtime,
                             const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() > 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (params.size() == 1 && runtime->ToWideString(params[0]) != L"pretty")
    return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

  // TODO(weili): Check whether we need to save pretty print XML, pdfium:501.

  ByteString bsXMLHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  if (GetXFANode()->GetPacketType() != XFA_PacketType::Form &&
      GetXFANode()->GetPacketType() != XFA_PacketType::Datasets) {
    return CJS_Return(runtime->NewString(""));
  }

  CFX_XMLNode* pElement = nullptr;
  if (GetXFANode()->GetPacketType() == XFA_PacketType::Datasets) {
    pElement = GetXFANode()->GetXMLMappingNode();
    if (!pElement || pElement->GetType() != FX_XMLNODE_Element) {
      return CJS_Return(runtime->NewString(bsXMLHeader.AsStringView()));
    }

    XFA_DataExporter_DealWithDataGroupNode(GetXFANode());
  }

  auto pMemoryStream = pdfium::MakeRetain<CFX_MemoryStream>(true);
  pMemoryStream->WriteString(bsXMLHeader.AsStringView());

  if (GetXFANode()->GetPacketType() == XFA_PacketType::Form) {
    XFA_DataExporter_RegenerateFormFile(GetXFANode(), pMemoryStream, true);
  } else {
    pElement->Save(pMemoryStream);
  }

  return CJS_Return(runtime->NewString(
      ByteStringView(pMemoryStream->GetBuffer(), pMemoryStream->GetSize())));
}

CJS_Return CJX_Node::setAttribute(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString attributeValue = runtime->ToWideString(params[0]);
  WideString attribute = runtime->ToWideString(params[1]);
  SetAttribute(attribute.AsStringView(), attributeValue.AsStringView(), true);
  return CJS_Return(true);
}

CJS_Return CJX_Node::setElement(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1 && params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  // TODO(weili): check whether we need to implement this, pdfium:501.
  return CJS_Return(true);
}

void CJX_Node::id(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  Script_Attribute_String(pValue, bSetting, eAttribute);
}

void CJX_Node::ns(CFXJSE_Value* pValue,
                  bool bSetting,
                  XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->SetString(
      TryNamespace().value_or(WideString()).UTF8Encode().AsStringView());
}

void CJX_Node::model(CFXJSE_Value* pValue,
                     bool bSetting,
                     XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->Assign(GetDocument()->GetScriptContext()->GetJSValueFromMap(
      GetXFANode()->GetModelNode()));
}

void CJX_Node::isContainer(CFXJSE_Value* pValue,
                           bool bSetting,
                           XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->SetBoolean(GetXFANode()->IsContainerNode());
}

void CJX_Node::isNull(CFXJSE_Value* pValue,
                      bool bSetting,
                      XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  if (GetXFANode()->GetElementType() == XFA_Element::Subform) {
    pValue->SetBoolean(false);
    return;
  }
  pValue->SetBoolean(GetContent(false).IsEmpty());
}

void CJX_Node::oneOfChild(CFXJSE_Value* pValue,
                          bool bSetting,
                          XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }

  std::vector<CXFA_Node*> properties = GetXFANode()->GetNodeList(
      XFA_NODEFILTER_OneOfProperty, XFA_Element::Unknown);
  if (!properties.empty()) {
    pValue->Assign(GetDocument()->GetScriptContext()->GetJSValueFromMap(
        properties.front()));
  }
}

int32_t CJX_Node::execSingleEventByName(const WideStringView& wsEventName,
                                        XFA_Element eType) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return XFA_EVENTERROR_NotExist;

  const XFA_ExecEventParaInfo* eventParaInfo =
      GetEventParaInfoByName(wsEventName);
  if (!eventParaInfo)
    return XFA_EVENTERROR_NotExist;

  switch (eventParaInfo->m_validFlags) {
    case EventAppliesToo::kNone:
      return XFA_EVENTERROR_NotExist;
    case EventAppliesToo::kAll:
    case EventAppliesToo::kAllNonRecursive:
      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false,
          eventParaInfo->m_validFlags == EventAppliesToo::kAll);
    case EventAppliesToo::kSubform:
      if (eType != XFA_Element::Subform)
        return XFA_EVENTERROR_NotExist;

      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false, false);
    case EventAppliesToo::kFieldOrExclusion: {
      if (eType != XFA_Element::ExclGroup && eType != XFA_Element::Field)
        return XFA_EVENTERROR_NotExist;

      CXFA_Node* pParentNode = GetXFANode()->GetParent();
      if (pParentNode &&
          pParentNode->GetElementType() == XFA_Element::ExclGroup) {
        // TODO(dsinclair): This seems like a bug, we do the same work twice?
        pNotify->ExecEventByDeepFirst(GetXFANode(), eventParaInfo->m_eventType,
                                      false, false);
      }
      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false, false);
    }
    case EventAppliesToo::kField:
      if (eType != XFA_Element::Field)
        return XFA_EVENTERROR_NotExist;

      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false, false);
    case EventAppliesToo::kSignature: {
      if (!GetXFANode()->IsWidgetReady())
        return XFA_EVENTERROR_NotExist;
      if (GetXFANode()->GetUIChildNode()->GetElementType() !=
          XFA_Element::Signature) {
        return XFA_EVENTERROR_NotExist;
      }
      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false, false);
    }
    case EventAppliesToo::kChoiceList: {
      if (!GetXFANode()->IsWidgetReady())
        return XFA_EVENTERROR_NotExist;
      if (GetXFANode()->GetUIChildNode()->GetElementType() !=
          XFA_Element::ChoiceList) {
        return XFA_EVENTERROR_NotExist;
      }
      return pNotify->ExecEventByDeepFirst(
          GetXFANode(), eventParaInfo->m_eventType, false, false);
    }
  }
  return XFA_EVENTERROR_NotExist;
}
