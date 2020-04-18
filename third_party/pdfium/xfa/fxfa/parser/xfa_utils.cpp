// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/xfa_utils.h"

#include <algorithm>
#include <vector>

#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/xml/cfx_xmlchardata.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "fxjs/xfa/cjx_object.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_localemgr.h"
#include "xfa/fxfa/parser/cxfa_localevalue.h"
#include "xfa/fxfa/parser/cxfa_measurement.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_ui.h"
#include "xfa/fxfa/parser/cxfa_value.h"
#include "xfa/fxfa/parser/xfa_basic_data.h"

namespace {

constexpr const char kFormNS[] = "http://www.xfa.org/schema/xfa-form/";

const double fraction_scales[] = {0.1,
                                  0.01,
                                  0.001,
                                  0.0001,
                                  0.00001,
                                  0.000001,
                                  0.0000001,
                                  0.00000001,
                                  0.000000001,
                                  0.0000000001,
                                  0.00000000001,
                                  0.000000000001,
                                  0.0000000000001,
                                  0.00000000000001,
                                  0.000000000000001,
                                  0.0000000000000001};

WideString ExportEncodeAttribute(const WideString& str) {
  CFX_WideTextBuf textBuf;
  int32_t iLen = str.GetLength();
  for (int32_t i = 0; i < iLen; i++) {
    switch (str[i]) {
      case '&':
        textBuf << L"&amp;";
        break;
      case '<':
        textBuf << L"&lt;";
        break;
      case '>':
        textBuf << L"&gt;";
        break;
      case '\'':
        textBuf << L"&apos;";
        break;
      case '\"':
        textBuf << L"&quot;";
        break;
      default:
        textBuf.AppendChar(str[i]);
    }
  }
  return textBuf.MakeString();
}

bool IsXMLValidChar(wchar_t ch) {
  return ch == 0x09 || ch == 0x0A || ch == 0x0D ||
         (ch >= 0x20 && ch <= 0xD7FF) || (ch >= 0xE000 && ch <= 0xFFFD);
}

WideString ExportEncodeContent(const WideString& str) {
  CFX_WideTextBuf textBuf;
  int32_t iLen = str.GetLength();
  for (int32_t i = 0; i < iLen; i++) {
    wchar_t ch = str[i];
    if (!IsXMLValidChar(ch))
      continue;

    if (ch == '&') {
      textBuf << L"&amp;";
    } else if (ch == '<') {
      textBuf << L"&lt;";
    } else if (ch == '>') {
      textBuf << L"&gt;";
    } else if (ch == '\'') {
      textBuf << L"&apos;";
    } else if (ch == '\"') {
      textBuf << L"&quot;";
    } else if (ch == ' ') {
      if (i && str[i - 1] != ' ') {
        textBuf.AppendChar(' ');
      } else {
        textBuf << L"&#x20;";
      }
    } else {
      textBuf.AppendChar(str[i]);
    }
  }
  return textBuf.MakeString();
}

bool AttributeSaveInDataModel(CXFA_Node* pNode, XFA_Attribute eAttribute) {
  bool bSaveInDataModel = false;
  if (pNode->GetElementType() != XFA_Element::Image)
    return bSaveInDataModel;

  CXFA_Node* pValueNode = pNode->GetParent();
  if (!pValueNode || pValueNode->GetElementType() != XFA_Element::Value)
    return bSaveInDataModel;

  CXFA_Node* pFieldNode = pValueNode->GetParent();
  if (pFieldNode && pFieldNode->GetBindData() &&
      eAttribute == XFA_Attribute::Href) {
    bSaveInDataModel = true;
  }
  return bSaveInDataModel;
}

bool ContentNodeNeedtoExport(CXFA_Node* pContentNode) {
  Optional<WideString> wsContent =
      pContentNode->JSObject()->TryContent(false, false);
  if (!wsContent)
    return false;

  ASSERT(pContentNode->IsContentNode());
  CXFA_Node* pParentNode = pContentNode->GetParent();
  if (!pParentNode || pParentNode->GetElementType() != XFA_Element::Value)
    return true;

  CXFA_Node* pGrandParentNode = pParentNode->GetParent();
  if (!pGrandParentNode || !pGrandParentNode->IsContainerNode())
    return true;
  if (!pGrandParentNode->GetBindData())
    return false;
  if (pGrandParentNode->GetFFWidgetType() == XFA_FFWidgetType::kPasswordEdit)
    return false;
  return true;
}

void SaveAttribute(CXFA_Node* pNode,
                   XFA_Attribute eName,
                   const WideString& wsName,
                   bool bProto,
                   WideString& wsOutput) {
  if (!bProto && !pNode->JSObject()->HasAttribute(eName))
    return;

  Optional<WideString> value = pNode->JSObject()->TryAttribute(eName, false);
  if (!value)
    return;

  wsOutput += L" ";
  wsOutput += wsName;
  wsOutput += L"=\"";
  wsOutput += ExportEncodeAttribute(*value);
  wsOutput += L"\"";
}

void RegenerateFormFile_Changed(CXFA_Node* pNode,
                                CFX_WideTextBuf& buf,
                                bool bSaveXML) {
  WideString wsAttrs;
  for (size_t i = 0;; ++i) {
    XFA_Attribute attr = pNode->GetAttribute(i);
    if (attr == XFA_Attribute::Unknown)
      break;

    if (attr == XFA_Attribute::Name ||
        (AttributeSaveInDataModel(pNode, attr) && !bSaveXML)) {
      continue;
    }
    WideString wsAttr;
    SaveAttribute(pNode, attr, CXFA_Node::AttributeToName(attr), bSaveXML,
                  wsAttr);
    wsAttrs += wsAttr;
  }

  WideString wsChildren;
  switch (pNode->GetObjectType()) {
    case XFA_ObjectType::ContentNode: {
      if (!bSaveXML && !ContentNodeNeedtoExport(pNode))
        break;

      CXFA_Node* pRawValueNode = pNode->GetFirstChild();
      while (pRawValueNode &&
             pRawValueNode->GetElementType() != XFA_Element::SharpxHTML &&
             pRawValueNode->GetElementType() != XFA_Element::Sharptext &&
             pRawValueNode->GetElementType() != XFA_Element::Sharpxml) {
        pRawValueNode = pRawValueNode->GetNextSibling();
      }
      if (!pRawValueNode)
        break;

      Optional<WideString> contentType =
          pNode->JSObject()->TryAttribute(XFA_Attribute::ContentType, false);
      if (pRawValueNode->GetElementType() == XFA_Element::SharpxHTML &&
          (contentType && *contentType == L"text/html")) {
        CFX_XMLNode* pExDataXML = pNode->GetXMLMappingNode();
        if (!pExDataXML)
          break;

        CFX_XMLNode* pRichTextXML = pExDataXML->GetFirstChild();
        if (!pRichTextXML)
          break;

        auto pMemStream = pdfium::MakeRetain<CFX_MemoryStream>(true);
        pRichTextXML->Save(pMemStream);
        wsChildren += WideString::FromUTF8(
            ByteStringView(pMemStream->GetBuffer(), pMemStream->GetSize()));
      } else if (pRawValueNode->GetElementType() == XFA_Element::Sharpxml &&
                 (contentType && *contentType == L"text/xml")) {
        Optional<WideString> rawValue = pRawValueNode->JSObject()->TryAttribute(
            XFA_Attribute::Value, false);
        if (!rawValue || rawValue->IsEmpty())
          break;

        std::vector<WideString> wsSelTextArray;
        size_t iStart = 0;
        auto iEnd = rawValue->Find(L'\n', iStart);
        iEnd = !iEnd.has_value() ? rawValue->GetLength() : iEnd;
        while (iEnd.has_value() && iEnd >= iStart) {
          wsSelTextArray.push_back(
              rawValue->Mid(iStart, iEnd.value() - iStart));
          iStart = iEnd.value() + 1;
          if (iStart >= rawValue->GetLength())
            break;
          iEnd = rawValue->Find(L'\n', iStart);
        }

        CXFA_Node* pParentNode = pNode->GetParent();
        ASSERT(pParentNode);
        CXFA_Node* pGrandparentNode = pParentNode->GetParent();
        ASSERT(pGrandparentNode);
        WideString bodyTagName;
        bodyTagName =
            pGrandparentNode->JSObject()->GetCData(XFA_Attribute::Name);
        if (bodyTagName.IsEmpty())
          bodyTagName = L"ListBox1";

        buf << L"<";
        buf << bodyTagName;
        buf << L" xmlns=\"\"\n>";
        for (int32_t i = 0; i < pdfium::CollectionSize<int32_t>(wsSelTextArray);
             i++) {
          buf << L"<value\n>";
          buf << ExportEncodeContent(wsSelTextArray[i]);
          buf << L"</value\n>";
        }
        buf << L"</";
        buf << bodyTagName;
        buf << L"\n>";
        wsChildren += buf.AsStringView();
        buf.Clear();
      } else {
        WideString wsValue =
            pRawValueNode->JSObject()->GetCData(XFA_Attribute::Value);
        wsChildren += ExportEncodeContent(wsValue);
      }
      break;
    }
    case XFA_ObjectType::TextNode:
    case XFA_ObjectType::NodeC:
    case XFA_ObjectType::NodeV: {
      WideString wsValue = pNode->JSObject()->GetCData(XFA_Attribute::Value);
      wsChildren += ExportEncodeContent(wsValue);
      break;
    }
    default:
      if (pNode->GetElementType() == XFA_Element::Items) {
        CXFA_Node* pTemplateNode = pNode->GetTemplateNodeIfExists();
        if (!pTemplateNode ||
            pTemplateNode->CountChildren(XFA_Element::Unknown, false) !=
                pNode->CountChildren(XFA_Element::Unknown, false)) {
          bSaveXML = true;
        }
      }
      CFX_WideTextBuf newBuf;
      CXFA_Node* pChildNode = pNode->GetFirstChild();
      while (pChildNode) {
        RegenerateFormFile_Changed(pChildNode, newBuf, bSaveXML);
        wsChildren += newBuf.AsStringView();
        newBuf.Clear();
        pChildNode = pChildNode->GetNextSibling();
      }
      if (!bSaveXML && !wsChildren.IsEmpty() &&
          pNode->GetElementType() == XFA_Element::Items) {
        wsChildren.clear();
        bSaveXML = true;
        CXFA_Node* pChild = pNode->GetFirstChild();
        while (pChild) {
          RegenerateFormFile_Changed(pChild, newBuf, bSaveXML);
          wsChildren += newBuf.AsStringView();
          newBuf.Clear();
          pChild = pChild->GetNextSibling();
        }
      }
      break;
  }

  if (!wsChildren.IsEmpty() || !wsAttrs.IsEmpty() ||
      pNode->JSObject()->HasAttribute(XFA_Attribute::Name)) {
    WideStringView wsElement = pNode->GetClassName();
    WideString wsName;
    SaveAttribute(pNode, XFA_Attribute::Name, L"name", true, wsName);
    buf << L"<";
    buf << wsElement;
    buf << wsName;
    buf << wsAttrs;
    if (wsChildren.IsEmpty()) {
      buf << L"\n/>";
    } else {
      buf << L"\n>";
      buf << wsChildren;
      buf << L"</";
      buf << wsElement;
      buf << L"\n>";
    }
  }
}

void RegenerateFormFile_Container(CXFA_Node* pNode,
                                  const RetainPtr<IFX_SeekableStream>& pStream,
                                  bool bSaveXML) {
  XFA_Element eType = pNode->GetElementType();
  if (eType == XFA_Element::Field || eType == XFA_Element::Draw ||
      !pNode->IsContainerNode()) {
    CFX_WideTextBuf buf;
    RegenerateFormFile_Changed(pNode, buf, bSaveXML);
    size_t nLen = buf.GetLength();
    if (nLen > 0)
      pStream->WriteString(buf.MakeString().UTF8Encode().AsStringView());
    return;
  }

  WideString wsElement(pNode->GetClassName());
  pStream->WriteString("<");
  pStream->WriteString(wsElement.UTF8Encode().AsStringView());

  WideString wsOutput;
  SaveAttribute(pNode, XFA_Attribute::Name, L"name", true, wsOutput);

  WideString wsAttrs;
  for (size_t i = 0;; ++i) {
    XFA_Attribute attr = pNode->GetAttribute(i);
    if (attr == XFA_Attribute::Unknown)
      break;
    if (attr == XFA_Attribute::Name)
      continue;

    WideString wsAttr;
    SaveAttribute(pNode, attr, CXFA_Node::AttributeToName(attr), false, wsAttr);
    wsOutput += wsAttr;
  }

  if (!wsOutput.IsEmpty())
    pStream->WriteString(wsOutput.UTF8Encode().AsStringView());

  CXFA_Node* pChildNode = pNode->GetFirstChild();
  if (!pChildNode) {
    pStream->WriteString(" />\n");
    return;
  }

  pStream->WriteString(">\n");
  while (pChildNode) {
    RegenerateFormFile_Container(pChildNode, pStream, bSaveXML);
    pChildNode = pChildNode->GetNextSibling();
  }
  pStream->WriteString("</");
  pStream->WriteString(wsElement.UTF8Encode().AsStringView());
  pStream->WriteString(">\n");
}

WideString RecognizeXFAVersionNumber(CXFA_Node* pTemplateRoot) {
  if (!pTemplateRoot)
    return WideString();

  Optional<WideString> templateNS = pTemplateRoot->JSObject()->TryNamespace();
  if (!templateNS)
    return WideString();

  XFA_VERSION eVersion =
      pTemplateRoot->GetDocument()->RecognizeXFAVersionNumber(*templateNS);
  if (eVersion == XFA_VERSION_UNKNOWN)
    eVersion = XFA_VERSION_DEFAULT;

  return WideString::Format(L"%i.%i", eVersion / 100, eVersion % 100);
}

}  // namespace

double XFA_GetFractionalScale(uint32_t idx) {
  return fraction_scales[idx];
}

int XFA_GetMaxFractionalScale() {
  return FX_ArraySize(fraction_scales);
}

CXFA_LocaleValue XFA_GetLocaleValue(CXFA_Node* pNode) {
  CXFA_Value* pNodeValue =
      pNode->GetChild<CXFA_Value>(0, XFA_Element::Value, false);
  if (!pNodeValue)
    return CXFA_LocaleValue();

  CXFA_Node* pValueChild = pNodeValue->GetFirstChild();
  if (!pValueChild)
    return CXFA_LocaleValue();

  int32_t iVTType = XFA_VT_NULL;
  switch (pValueChild->GetElementType()) {
    case XFA_Element::Decimal:
      iVTType = XFA_VT_DECIMAL;
      break;
    case XFA_Element::Float:
      iVTType = XFA_VT_FLOAT;
      break;
    case XFA_Element::Date:
      iVTType = XFA_VT_DATE;
      break;
    case XFA_Element::Time:
      iVTType = XFA_VT_TIME;
      break;
    case XFA_Element::DateTime:
      iVTType = XFA_VT_DATETIME;
      break;
    case XFA_Element::Boolean:
      iVTType = XFA_VT_BOOLEAN;
      break;
    case XFA_Element::Integer:
      iVTType = XFA_VT_INTEGER;
      break;
    case XFA_Element::Text:
      iVTType = XFA_VT_TEXT;
      break;
    default:
      iVTType = XFA_VT_NULL;
      break;
  }
  return CXFA_LocaleValue(iVTType, pNode->GetRawValue(),
                          pNode->GetDocument()->GetLocalMgr());
}

bool XFA_FDEExtension_ResolveNamespaceQualifier(CFX_XMLElement* pNode,
                                                const WideString& wsQualifier,
                                                WideString* wsNamespaceURI) {
  if (!pNode)
    return false;

  CFX_XMLNode* pFakeRoot = pNode->GetRoot();
  WideString wsNSAttribute;
  bool bRet = false;
  if (wsQualifier.IsEmpty()) {
    wsNSAttribute = L"xmlns";
    bRet = true;
  } else {
    wsNSAttribute = L"xmlns:" + wsQualifier;
  }
  for (CFX_XMLNode* pParent = pNode; pParent != pFakeRoot;
       pParent = pParent->GetParent()) {
    if (pParent->GetType() != FX_XMLNODE_Element)
      continue;

    auto* pElement = static_cast<CFX_XMLElement*>(pParent);
    if (pElement->HasAttribute(wsNSAttribute.c_str())) {
      *wsNamespaceURI = pElement->GetAttribute(wsNSAttribute.c_str());
      return true;
    }
  }
  wsNamespaceURI->clear();
  return bRet;
}

void XFA_DataExporter_DealWithDataGroupNode(CXFA_Node* pDataNode) {
  if (!pDataNode || pDataNode->GetElementType() == XFA_Element::DataValue)
    return;

  int32_t iChildNum = 0;
  for (CXFA_Node* pChildNode = pDataNode->GetFirstChild(); pChildNode;
       pChildNode = pChildNode->GetNextSibling()) {
    iChildNum++;
    XFA_DataExporter_DealWithDataGroupNode(pChildNode);
  }

  if (pDataNode->GetElementType() != XFA_Element::DataGroup)
    return;

  if (iChildNum > 0) {
    CFX_XMLNode* pXMLNode = pDataNode->GetXMLMappingNode();
    ASSERT(pXMLNode->GetType() == FX_XMLNODE_Element);
    CFX_XMLElement* pXMLElement = static_cast<CFX_XMLElement*>(pXMLNode);
    if (pXMLElement->HasAttribute(L"xfa:dataNode"))
      pXMLElement->RemoveAttribute(L"xfa:dataNode");

    return;
  }

  CFX_XMLNode* pXMLNode = pDataNode->GetXMLMappingNode();
  ASSERT(pXMLNode->GetType() == FX_XMLNODE_Element);
  static_cast<CFX_XMLElement*>(pXMLNode)->SetAttribute(L"xfa:dataNode",
                                                       L"dataGroup");
}

void XFA_DataExporter_RegenerateFormFile(
    CXFA_Node* pNode,
    const RetainPtr<IFX_SeekableStream>& pStream,
    bool bSaveXML) {
  if (pNode->IsModelNode()) {
    pStream->WriteString("<form xmlns=\"");
    pStream->WriteString(kFormNS);

    WideString wsVersionNumber = RecognizeXFAVersionNumber(
        ToNode(pNode->GetDocument()->GetXFAObject(XFA_HASHCODE_Template)));
    if (wsVersionNumber.IsEmpty())
      wsVersionNumber = L"2.8";

    wsVersionNumber += L"/\"\n>";
    pStream->WriteString(wsVersionNumber.UTF8Encode().AsStringView());

    CXFA_Node* pChildNode = pNode->GetFirstChild();
    while (pChildNode) {
      RegenerateFormFile_Container(pChildNode, pStream, false);
      pChildNode = pChildNode->GetNextSibling();
    }
    pStream->WriteString("</form\n>");
  } else {
    RegenerateFormFile_Container(pNode, pStream, bSaveXML);
  }
}

bool XFA_FieldIsMultiListBox(CXFA_Node* pFieldNode) {
  if (!pFieldNode)
    return false;

  CXFA_Ui* pUIChild = pFieldNode->GetChild<CXFA_Ui>(0, XFA_Element::Ui, false);
  if (!pUIChild)
    return false;

  CXFA_Node* pFirstChild = pUIChild->GetFirstChild();
  if (!pFirstChild ||
      pFirstChild->GetElementType() != XFA_Element::ChoiceList) {
    return false;
  }

  return pFirstChild->JSObject()->GetEnum(XFA_Attribute::Open) ==
         XFA_AttributeEnum::MultiSelect;
}

int32_t XFA_MapRotation(int32_t nRotation) {
  nRotation = nRotation % 360;
  nRotation = nRotation < 0 ? nRotation + 360 : nRotation;
  return nRotation;
}

const XFA_SCRIPTATTRIBUTEINFO* XFA_GetScriptAttributeByName(
    XFA_Element eElement,
    WideStringView wsAttributeName) {
  if (wsAttributeName.IsEmpty())
    return nullptr;

  int32_t iElementIndex = static_cast<int32_t>(eElement);
  while (iElementIndex != -1) {
    const XFA_SCRIPTHIERARCHY* scriptIndex = g_XFAScriptIndex + iElementIndex;
    size_t iCount = scriptIndex->wAttributeCount;
    if (iCount == 0) {
      iElementIndex = scriptIndex->wParentIndex;
      continue;
    }

    uint32_t uHash = FX_HashCode_GetW(wsAttributeName, false);
    size_t iStart = scriptIndex->wAttributeStart;
    size_t iEnd = iStart + iCount;
    for (size_t iter = iStart; iter < iEnd; ++iter) {
      const XFA_SCRIPTATTRIBUTEINFO* pInfo = g_SomAttributeData + iter;
      if (uHash == pInfo->uHash)
        return pInfo;
    }

    iElementIndex = scriptIndex->wParentIndex;
  }
  return nullptr;
}
