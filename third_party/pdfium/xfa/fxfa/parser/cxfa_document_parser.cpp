// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_document_parser.h"

#include <utility>
#include <vector>

#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/xml/cfx_xmlchardata.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlinstruction.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"
#include "core/fxcrt/xml/cfx_xmlparser.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "fxjs/xfa/cjx_object.h"
#include "third_party/base/logging.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/fxfa.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_subform.h"
#include "xfa/fxfa/parser/cxfa_template.h"
#include "xfa/fxfa/parser/xfa_basic_data.h"
#include "xfa/fxfa/parser/xfa_utils.h"

namespace {

struct PacketInfo {
  uint32_t hash;
  const wchar_t* name;
  XFA_PacketType packet_type;
  const wchar_t* uri;
  uint32_t flags;
};
const PacketInfo PacketData[] = {
    {0x0, nullptr, XFA_PacketType::User, nullptr,
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTMANY},
    {0x811929d, L"sourceSet", XFA_PacketType::SourceSet,
     L"http://www.xfa.org/schema/xfa-source-set/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0xb843dba, L"pdf", XFA_PacketType::Pdf, L"http://ns.adobe.com/xdp/pdf/",
     XFA_XDPPACKET_FLAGS_COMPLETEMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0xc56afbf, L"xdc", XFA_PacketType::Xdc, L"http://www.xfa.org/schema/xdc/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0xc56afcc, L"xdp", XFA_PacketType::Xdp, L"http://ns.adobe.com/xdp/",
     XFA_XDPPACKET_FLAGS_COMPLETEMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x132a8fbc, L"xmpmeta", XFA_PacketType::Xmpmeta,
     L"http://ns.adobe.com/xmpmeta/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTMANY},
    {0x48d004a8, L"xfdf", XFA_PacketType::Xfdf, L"http://ns.adobe.com/xfdf/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x4e1e39b6, L"config", XFA_PacketType::Config,
     L"http://www.xfa.org/schema/xci/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x5473b6dc, L"localeSet", XFA_PacketType::LocaleSet,
     L"http://www.xfa.org/schema/xfa-locale-set/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x6038580a, L"stylesheet", XFA_PacketType::Stylesheet,
     L"http://www.w3.org/1999/XSL/Transform",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTMANY},
    {0x803550fc, L"template", XFA_PacketType::Template,
     L"http://www.xfa.org/schema/xfa-template/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x8b036f32, L"signature", XFA_PacketType::Signature,
     L"http://www.w3.org/2000/09/xmldsig#",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0x99b95079, L"datasets", XFA_PacketType::Datasets,
     L"http://www.xfa.org/schema/xfa-data/",
     XFA_XDPPACKET_FLAGS_PREFIXMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0xcd309ff4, L"form", XFA_PacketType::Form,
     L"http://www.xfa.org/schema/xfa-form/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
    {0xe14c801c, L"connectionSet", XFA_PacketType::ConnectionSet,
     L"http://www.xfa.org/schema/xfa-connection-set/",
     XFA_XDPPACKET_FLAGS_NOMATCH | XFA_XDPPACKET_FLAGS_SUPPORTONE},
};

const PacketInfo* GetPacketByIndex(XFA_PacketType ePacket) {
  return PacketData + static_cast<uint8_t>(ePacket);
}

const PacketInfo* GetPacketByName(const WideStringView& wsName) {
  if (wsName.IsEmpty())
    return nullptr;

  uint32_t hash = FX_HashCode_GetW(wsName, false);
  auto* elem = std::lower_bound(
      std::begin(PacketData), std::end(PacketData), hash,
      [](const PacketInfo& a, uint32_t hash) { return a.hash < hash; });
  if (elem != std::end(PacketData) && elem->hash == hash)
    return elem;
  return nullptr;
}

CFX_XMLNode* GetDocumentNode(CFX_XMLNode* pRootNode) {
  for (CFX_XMLNode* pXMLNode = pRootNode->GetFirstChild(); pXMLNode;
       pXMLNode = pXMLNode->GetNextSibling()) {
    if (pXMLNode->GetType() != FX_XMLNODE_Element)
      continue;

    return pXMLNode;
  }
  return nullptr;
}

bool MatchNodeName(CFX_XMLNode* pNode,
                   const WideStringView& wsLocalTagName,
                   const WideStringView& wsNamespaceURIPrefix,
                   uint32_t eMatchFlags = XFA_XDPPACKET_FLAGS_NOMATCH) {
  if (!pNode || pNode->GetType() != FX_XMLNODE_Element)
    return false;

  CFX_XMLElement* pElement = static_cast<CFX_XMLElement*>(pNode);
  WideString wsNodeStr = pElement->GetLocalTagName();
  if (wsNodeStr != wsLocalTagName)
    return false;

  wsNodeStr = pElement->GetNamespaceURI();
  if (eMatchFlags & XFA_XDPPACKET_FLAGS_NOMATCH)
    return true;
  if (eMatchFlags & XFA_XDPPACKET_FLAGS_PREFIXMATCH) {
    return wsNodeStr.Left(wsNamespaceURIPrefix.GetLength()) ==
           wsNamespaceURIPrefix;
  }

  return wsNodeStr == wsNamespaceURIPrefix;
}

bool GetAttributeLocalName(const WideStringView& wsAttributeName,
                           WideString& wsLocalAttrName) {
  WideString wsAttrName(wsAttributeName);
  auto pos = wsAttrName.Find(L':', 0);
  if (!pos.has_value()) {
    wsLocalAttrName = wsAttrName;
    return false;
  }
  wsLocalAttrName = wsAttrName.Right(wsAttrName.GetLength() - pos.value() - 1);
  return true;
}

bool ResolveAttribute(CFX_XMLElement* pElement,
                      const WideString& wsAttrName,
                      WideString& wsLocalAttrName,
                      WideString& wsNamespaceURI) {
  WideString wsNSPrefix;
  if (GetAttributeLocalName(wsAttrName.AsStringView(), wsLocalAttrName)) {
    wsNSPrefix = wsAttrName.Left(wsAttrName.GetLength() -
                                 wsLocalAttrName.GetLength() - 1);
  }
  if (wsLocalAttrName == L"xmlns" || wsNSPrefix == L"xmlns" ||
      wsNSPrefix == L"xml") {
    return false;
  }
  if (!XFA_FDEExtension_ResolveNamespaceQualifier(pElement, wsNSPrefix,
                                                  &wsNamespaceURI)) {
    wsNamespaceURI.clear();
    return false;
  }
  return true;
}

bool FindAttributeWithNS(CFX_XMLElement* pElement,
                         const WideStringView& wsLocalAttributeName,
                         const WideStringView& wsNamespaceURIPrefix,
                         WideString& wsValue,
                         bool bMatchNSAsPrefix = false) {
  if (!pElement)
    return false;

  WideString wsAttrNS;
  for (auto it : pElement->GetAttributes()) {
    auto pos = it.first.Find(L':', 0);
    WideString wsNSPrefix;
    if (!pos.has_value()) {
      if (wsLocalAttributeName != it.first)
        continue;
    } else {
      if (wsLocalAttributeName !=
          it.first.Right(it.first.GetLength() - pos.value() - 1)) {
        continue;
      }
      wsNSPrefix = it.first.Left(pos.value());
    }

    if (!XFA_FDEExtension_ResolveNamespaceQualifier(pElement, wsNSPrefix,
                                                    &wsAttrNS)) {
      continue;
    }
    if (bMatchNSAsPrefix) {
      if (wsAttrNS.Left(wsNamespaceURIPrefix.GetLength()) !=
          wsNamespaceURIPrefix) {
        continue;
      }
    } else {
      if (wsAttrNS != wsNamespaceURIPrefix)
        continue;
    }
    wsValue = it.second;
    return true;
  }
  return false;
}

CFX_XMLNode* GetDataSetsFromXDP(CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* datasets_packet =
      GetPacketByIndex(XFA_PacketType::Datasets);
  if (MatchNodeName(pXMLDocumentNode, datasets_packet->name,
                    datasets_packet->uri, datasets_packet->flags)) {
    return pXMLDocumentNode;
  }

  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Xdp);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }

  for (CFX_XMLNode* pDatasetsNode = pXMLDocumentNode->GetFirstChild();
       pDatasetsNode; pDatasetsNode = pDatasetsNode->GetNextSibling()) {
    if (MatchNodeName(pDatasetsNode, datasets_packet->name,
                      datasets_packet->uri, datasets_packet->flags)) {
      return pDatasetsNode;
    }
  }
  return nullptr;
}

bool IsStringAllWhitespace(WideString wsText) {
  wsText.TrimRight(L"\x20\x9\xD\xA");
  return wsText.IsEmpty();
}

void ConvertXMLToPlainText(CFX_XMLElement* pRootXMLNode, WideString& wsOutput) {
  for (CFX_XMLNode* pXMLChild = pRootXMLNode->GetFirstChild(); pXMLChild;
       pXMLChild = pXMLChild->GetNextSibling()) {
    switch (pXMLChild->GetType()) {
      case FX_XMLNODE_Element: {
        WideString wsTextData =
            static_cast<CFX_XMLElement*>(pXMLChild)->GetTextData();
        wsTextData += L"\n";
        wsOutput += wsTextData;
        break;
      }
      case FX_XMLNODE_Text:
      case FX_XMLNODE_CharData: {
        WideString wsText = static_cast<CFX_XMLText*>(pXMLChild)->GetText();
        if (IsStringAllWhitespace(wsText))
          continue;

        wsOutput = wsText;
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }
}

WideString GetPlainTextFromRichText(CFX_XMLNode* pXMLNode) {
  if (!pXMLNode)
    return L"";

  WideString wsPlainText;
  switch (pXMLNode->GetType()) {
    case FX_XMLNODE_Element: {
      CFX_XMLElement* pXMLElement = static_cast<CFX_XMLElement*>(pXMLNode);
      WideString wsTag = pXMLElement->GetLocalTagName();
      uint32_t uTag = FX_HashCode_GetW(wsTag.AsStringView(), true);
      if (uTag == 0x0001f714) {
        wsPlainText += L"\n";
      } else if (uTag == 0x00000070) {
        if (!wsPlainText.IsEmpty()) {
          wsPlainText += L"\n";
        }
      } else if (uTag == 0xa48ac63) {
        if (!wsPlainText.IsEmpty() &&
            wsPlainText[wsPlainText.GetLength() - 1] != '\n') {
          wsPlainText += L"\n";
        }
      }
      break;
    }
    case FX_XMLNODE_Text:
    case FX_XMLNODE_CharData: {
      WideString wsContent = static_cast<CFX_XMLText*>(pXMLNode)->GetText();
      wsPlainText += wsContent;
      break;
    }
    default:
      break;
  }
  for (CFX_XMLNode* pChildXML = pXMLNode->GetFirstChild(); pChildXML;
       pChildXML = pChildXML->GetNextSibling()) {
    wsPlainText += GetPlainTextFromRichText(pChildXML);
  }

  return wsPlainText;
}

}  // namespace

bool XFA_RecognizeRichText(CFX_XMLElement* pRichTextXMLNode) {
  return pRichTextXMLNode &&
         pRichTextXMLNode->GetNamespaceURI() == L"http://www.w3.org/1999/xhtml";
}

CXFA_DocumentParser::CXFA_DocumentParser(CXFA_Document* pFactory)
    : m_pFactory(pFactory) {}

CXFA_DocumentParser::~CXFA_DocumentParser() = default;

bool CXFA_DocumentParser::Parse(const RetainPtr<IFX_SeekableStream>& pStream,
                                XFA_PacketType ePacketID) {
  xml_doc_ = LoadXML(pStream);
  if (!xml_doc_)
    return false;

  CFX_XMLNode* root = GetDocumentNode(xml_doc_->GetRoot());
  if (!root)
    return false;

  m_pRootNode = ParseAsXDPPacket(root, ePacketID);
  return !!m_pRootNode;
}

CFX_XMLNode* CXFA_DocumentParser::ParseXMLData(const ByteString& wsXML) {
  auto pStream = pdfium::MakeRetain<CFX_MemoryStream>(
      const_cast<uint8_t*>(wsXML.raw_str()), wsXML.GetLength(), false);
  xml_doc_ = LoadXML(pStream);
  if (!xml_doc_)
    return nullptr;
  return GetDocumentNode(xml_doc_->GetRoot());
}

std::unique_ptr<CFX_XMLDocument> CXFA_DocumentParser::LoadXML(
    const RetainPtr<IFX_SeekableStream>& pStream) {
  ASSERT(pStream);

  CFX_XMLParser parser(pStream);
  std::unique_ptr<CFX_XMLDocument> doc = parser.Parse();
  if (doc) {
    doc->GetRoot()->InsertChildNode(doc->CreateNode<CFX_XMLInstruction>(L"xml"),
                                    0);
  }
  return doc;
}

void CXFA_DocumentParser::ConstructXFANode(CXFA_Node* pXFANode,
                                           CFX_XMLNode* pXMLNode) {
  XFA_PacketType ePacketID = pXFANode->GetPacketType();
  if (ePacketID == XFA_PacketType::Datasets) {
    if (pXFANode->GetElementType() == XFA_Element::DataValue) {
      for (CFX_XMLNode* pXMLChild = pXMLNode->GetFirstChild(); pXMLChild;
           pXMLChild = pXMLChild->GetNextSibling()) {
        FX_XMLNODETYPE eNodeType = pXMLChild->GetType();
        if (eNodeType == FX_XMLNODE_Instruction)
          continue;

        if (eNodeType == FX_XMLNODE_Element) {
          CXFA_Node* pXFAChild = m_pFactory->CreateNode(
              XFA_PacketType::Datasets, XFA_Element::DataValue);
          if (!pXFAChild)
            return;

          CFX_XMLElement* child = static_cast<CFX_XMLElement*>(pXMLChild);
          WideString wsNodeStr = child->GetLocalTagName();
          pXFAChild->JSObject()->SetCData(XFA_Attribute::Name, wsNodeStr, false,
                                          false);
          WideString wsChildValue = GetPlainTextFromRichText(child);
          if (!wsChildValue.IsEmpty())
            pXFAChild->JSObject()->SetCData(XFA_Attribute::Value, wsChildValue,
                                            false, false);

          pXFANode->InsertChild(pXFAChild, nullptr);
          pXFAChild->SetXMLMappingNode(pXMLChild);
          pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
          break;
        }
      }
      m_pRootNode = pXFANode;
    } else {
      m_pRootNode = DataLoader(pXFANode, pXMLNode, true);
    }
  } else if (pXFANode->IsContentNode()) {
    ParseContentNode(pXFANode, pXMLNode, ePacketID);
    m_pRootNode = pXFANode;
  } else {
    m_pRootNode = NormalLoader(pXFANode, pXMLNode, ePacketID, true);
  }
}

CXFA_Node* CXFA_DocumentParser::GetRootNode() const {
  return m_pRootNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket(CFX_XMLNode* pXMLDocumentNode,
                                                 XFA_PacketType ePacketID) {
  switch (ePacketID) {
    case XFA_PacketType::Xdp:
      return ParseAsXDPPacket_XDP(pXMLDocumentNode);
    case XFA_PacketType::Config:
      return ParseAsXDPPacket_Config(pXMLDocumentNode);
    case XFA_PacketType::Template:
      return ParseAsXDPPacket_Template(pXMLDocumentNode);
    case XFA_PacketType::Form:
      return ParseAsXDPPacket_Form(pXMLDocumentNode);
    case XFA_PacketType::Datasets:
      return ParseAsXDPPacket_Data(pXMLDocumentNode);
    case XFA_PacketType::Xdc:
      return ParseAsXDPPacket_Xdc(pXMLDocumentNode);
    case XFA_PacketType::LocaleSet:
      return ParseAsXDPPacket_LocaleConnectionSourceSet(
          pXMLDocumentNode, XFA_PacketType::LocaleSet, XFA_Element::LocaleSet);
    case XFA_PacketType::ConnectionSet:
      return ParseAsXDPPacket_LocaleConnectionSourceSet(
          pXMLDocumentNode, XFA_PacketType::ConnectionSet,
          XFA_Element::ConnectionSet);
    case XFA_PacketType::SourceSet:
      return ParseAsXDPPacket_LocaleConnectionSourceSet(
          pXMLDocumentNode, XFA_PacketType::SourceSet, XFA_Element::SourceSet);
    default:
      return ParseAsXDPPacket_User(pXMLDocumentNode);
  }
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_XDP(
    CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Xdp);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }

  CXFA_Node* pXFARootNode =
      m_pFactory->CreateNode(XFA_PacketType::Xdp, XFA_Element::Xfa);
  if (!pXFARootNode)
    return nullptr;

  m_pRootNode = pXFARootNode;
  pXFARootNode->JSObject()->SetCData(XFA_Attribute::Name, L"xfa", false, false);

  CFX_XMLElement* pElement = static_cast<CFX_XMLElement*>(pXMLDocumentNode);
  for (auto it : pElement->GetAttributes()) {
    if (it.first == L"uuid")
      pXFARootNode->JSObject()->SetCData(XFA_Attribute::Uuid, it.second, false,
                                         false);
    else if (it.first == L"timeStamp")
      pXFARootNode->JSObject()->SetCData(XFA_Attribute::TimeStamp, it.second,
                                         false, false);
  }

  CFX_XMLNode* pXMLConfigDOMRoot = nullptr;
  CXFA_Node* pXFAConfigDOMRoot = nullptr;
  const PacketInfo* config_packet_info =
      GetPacketByIndex(XFA_PacketType::Config);
  for (CFX_XMLNode* pChildItem = pXMLDocumentNode->GetFirstChild(); pChildItem;
       pChildItem = pChildItem->GetNextSibling()) {
    if (!MatchNodeName(pChildItem, config_packet_info->name,
                       config_packet_info->uri, config_packet_info->flags)) {
      continue;
    }
    if (pXFARootNode->GetFirstChildByName(config_packet_info->hash))
      return nullptr;

    pXMLConfigDOMRoot = pChildItem;
    pXFAConfigDOMRoot = ParseAsXDPPacket_Config(pXMLConfigDOMRoot);
    if (pXFAConfigDOMRoot)
      pXFARootNode->InsertChild(pXFAConfigDOMRoot, nullptr);
  }

  CFX_XMLNode* pXMLDatasetsDOMRoot = nullptr;
  CFX_XMLNode* pXMLFormDOMRoot = nullptr;
  CFX_XMLNode* pXMLTemplateDOMRoot = nullptr;
  for (CFX_XMLNode* pChildItem = pXMLDocumentNode->GetFirstChild(); pChildItem;
       pChildItem = pChildItem->GetNextSibling()) {
    if (!pChildItem || pChildItem->GetType() != FX_XMLNODE_Element)
      continue;
    if (pChildItem == pXMLConfigDOMRoot)
      continue;

    CFX_XMLElement* pElement = static_cast<CFX_XMLElement*>(pChildItem);
    WideString wsPacketName = pElement->GetLocalTagName();
    const PacketInfo* pPacketInfo =
        GetPacketByName(wsPacketName.AsStringView());
    if (pPacketInfo && pPacketInfo->uri) {
      if (!MatchNodeName(pElement, pPacketInfo->name, pPacketInfo->uri,
                         pPacketInfo->flags)) {
        pPacketInfo = nullptr;
      }
    }
    XFA_PacketType ePacket =
        pPacketInfo ? pPacketInfo->packet_type : XFA_PacketType::User;
    if (ePacket == XFA_PacketType::Xdp)
      continue;
    if (ePacket == XFA_PacketType::Datasets) {
      if (pXMLDatasetsDOMRoot)
        return nullptr;

      pXMLDatasetsDOMRoot = pElement;
    } else if (ePacket == XFA_PacketType::Form) {
      if (pXMLFormDOMRoot)
        return nullptr;

      pXMLFormDOMRoot = pElement;
    } else if (ePacket == XFA_PacketType::Template) {
      // Found a duplicate template packet.
      if (pXMLTemplateDOMRoot)
        return nullptr;

      CXFA_Node* pPacketNode = ParseAsXDPPacket_Template(pElement);
      if (pPacketNode) {
        pXMLTemplateDOMRoot = pElement;
        pXFARootNode->InsertChild(pPacketNode, nullptr);
      }
    } else {
      CXFA_Node* pPacketNode = ParseAsXDPPacket(pElement, ePacket);
      if (pPacketNode) {
        if (pPacketInfo &&
            (pPacketInfo->flags & XFA_XDPPACKET_FLAGS_SUPPORTONE) &&
            pXFARootNode->GetFirstChildByName(pPacketInfo->hash)) {
          return nullptr;
        }
        pXFARootNode->InsertChild(pPacketNode, nullptr);
      }
    }
  }

  // No template is found.
  if (!pXMLTemplateDOMRoot)
    return nullptr;

  if (pXMLDatasetsDOMRoot) {
    CXFA_Node* pPacketNode =
        ParseAsXDPPacket(pXMLDatasetsDOMRoot, XFA_PacketType::Datasets);
    if (pPacketNode)
      pXFARootNode->InsertChild(pPacketNode, nullptr);
  }
  if (pXMLFormDOMRoot) {
    CXFA_Node* pPacketNode =
        ParseAsXDPPacket(pXMLFormDOMRoot, XFA_PacketType::Form);
    if (pPacketNode)
      pXFARootNode->InsertChild(pPacketNode, nullptr);
  }

  pXFARootNode->SetXMLMappingNode(pXMLDocumentNode);
  return pXFARootNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_Config(
    CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Config);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }
  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Config, XFA_Element::Config);
  if (!pNode)
    return nullptr;

  pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false, false);
  if (!NormalLoader(pNode, pXMLDocumentNode, XFA_PacketType::Config, true))
    return nullptr;

  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_Template(
    CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Template);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }

  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Template, XFA_Element::Template);
  if (!pNode)
    return nullptr;

  pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false, false);

  CFX_XMLElement* pXMLDocumentElement =
      static_cast<CFX_XMLElement*>(pXMLDocumentNode);
  WideString wsNamespaceURI = pXMLDocumentElement->GetNamespaceURI();
  if (wsNamespaceURI.IsEmpty())
    wsNamespaceURI = pXMLDocumentElement->GetAttribute(L"xmlns:xfa");

  pNode->GetDocument()->RecognizeXFAVersionNumber(wsNamespaceURI);

  if (!NormalLoader(pNode, pXMLDocumentNode, XFA_PacketType::Template, true))
    return nullptr;

  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_Form(
    CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Form);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }

  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Form, XFA_Element::Form);
  if (!pNode)
    return nullptr;

  pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false, false);
  CXFA_Template* pTemplateRoot =
      m_pRootNode->GetFirstChildByClass<CXFA_Template>(XFA_Element::Template);
  CXFA_Subform* pTemplateChosen =
      pTemplateRoot ? pTemplateRoot->GetFirstChildByClass<CXFA_Subform>(
                          XFA_Element::Subform)
                    : nullptr;
  bool bUseAttribute = true;
  if (pTemplateChosen &&
      pTemplateChosen->JSObject()->GetEnum(XFA_Attribute::RestoreState) !=
          XFA_AttributeEnum::Auto) {
    bUseAttribute = false;
  }
  if (!NormalLoader(pNode, pXMLDocumentNode, XFA_PacketType::Form,
                    bUseAttribute))
    return nullptr;

  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_Data(
    CFX_XMLNode* pXMLDocumentNode) {
  CFX_XMLNode* pDatasetsXMLNode = GetDataSetsFromXDP(pXMLDocumentNode);
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Datasets);
  if (pDatasetsXMLNode) {
    CXFA_Node* pNode = m_pFactory->CreateNode(XFA_PacketType::Datasets,
                                              XFA_Element::DataModel);
    if (!pNode)
      return nullptr;

    pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false,
                                false);
    if (!DataLoader(pNode, pDatasetsXMLNode, false))
      return nullptr;

    pNode->SetXMLMappingNode(pDatasetsXMLNode);
    return pNode;
  }

  CFX_XMLNode* pDataXMLNode = nullptr;
  if (MatchNodeName(pXMLDocumentNode, L"data", packet->uri, packet->flags)) {
    static_cast<CFX_XMLElement*>(pXMLDocumentNode)
        ->RemoveAttribute(L"xmlns:xfa");
    pDataXMLNode = pXMLDocumentNode;
  } else {
    auto* pDataElement = xml_doc_->CreateNode<CFX_XMLElement>(L"xfa:data");
    CFX_XMLNode* pParentXMLNode = pXMLDocumentNode->GetParent();
    if (pParentXMLNode)
      pParentXMLNode->RemoveChildNode(pXMLDocumentNode);

    ASSERT(pXMLDocumentNode->GetType() == FX_XMLNODE_Element);
    if (pXMLDocumentNode->GetType() == FX_XMLNODE_Element) {
      static_cast<CFX_XMLElement*>(pXMLDocumentNode)
          ->RemoveAttribute(L"xmlns:xfa");
    }
    // The node was either removed from the parent above, or already has no
    // parent so we can take ownership.
    pDataElement->AppendChild(pXMLDocumentNode);
    pDataXMLNode = pDataElement;
  }
  if (!pDataXMLNode)
    return nullptr;

  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Datasets, XFA_Element::DataGroup);
  if (!pNode)
    return nullptr;

  WideString wsLocalName =
      static_cast<CFX_XMLElement*>(pDataXMLNode)->GetLocalTagName();
  pNode->JSObject()->SetCData(XFA_Attribute::Name, wsLocalName, false, false);
  if (!DataLoader(pNode, pDataXMLNode, true))
    return nullptr;

  pNode->SetXMLMappingNode(pDataXMLNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_LocaleConnectionSourceSet(
    CFX_XMLNode* pXMLDocumentNode,
    XFA_PacketType packet_type,
    XFA_Element element) {
  const PacketInfo* packet = GetPacketByIndex(packet_type);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags)) {
    return nullptr;
  }

  CXFA_Node* pNode = m_pFactory->CreateNode(packet_type, element);
  if (!pNode)
    return nullptr;

  pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false, false);
  if (!NormalLoader(pNode, pXMLDocumentNode, packet_type, true))
    return nullptr;

  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_Xdc(
    CFX_XMLNode* pXMLDocumentNode) {
  const PacketInfo* packet = GetPacketByIndex(XFA_PacketType::Xdc);
  if (!MatchNodeName(pXMLDocumentNode, packet->name, packet->uri,
                     packet->flags))
    return nullptr;

  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Xdc, XFA_Element::Xdc);
  if (!pNode)
    return nullptr;

  pNode->JSObject()->SetCData(XFA_Attribute::Name, packet->name, false, false);
  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::ParseAsXDPPacket_User(
    CFX_XMLNode* pXMLDocumentNode) {
  CXFA_Node* pNode =
      m_pFactory->CreateNode(XFA_PacketType::Xdp, XFA_Element::Packet);
  if (!pNode)
    return nullptr;

  WideString wsName =
      static_cast<CFX_XMLElement*>(pXMLDocumentNode)->GetLocalTagName();
  pNode->JSObject()->SetCData(XFA_Attribute::Name, wsName, false, false);
  if (!UserPacketLoader(pNode, pXMLDocumentNode))
    return nullptr;

  pNode->SetXMLMappingNode(pXMLDocumentNode);
  return pNode;
}

CXFA_Node* CXFA_DocumentParser::UserPacketLoader(CXFA_Node* pXFANode,
                                                 CFX_XMLNode* pXMLDoc) {
  return pXFANode;
}

CXFA_Node* CXFA_DocumentParser::DataLoader(CXFA_Node* pXFANode,
                                           CFX_XMLNode* pXMLDoc,
                                           bool bDoTransform) {
  ParseDataGroup(pXFANode, pXMLDoc, XFA_PacketType::Datasets);
  return pXFANode;
}

CXFA_Node* CXFA_DocumentParser::NormalLoader(CXFA_Node* pXFANode,
                                             CFX_XMLNode* pXMLDoc,
                                             XFA_PacketType ePacketID,
                                             bool bUseAttribute) {
  bool bOneOfPropertyFound = false;
  for (CFX_XMLNode* pXMLChild = pXMLDoc->GetFirstChild(); pXMLChild;
       pXMLChild = pXMLChild->GetNextSibling()) {
    switch (pXMLChild->GetType()) {
      case FX_XMLNODE_Element: {
        CFX_XMLElement* pXMLElement = static_cast<CFX_XMLElement*>(pXMLChild);
        WideString wsTagName = pXMLElement->GetLocalTagName();
        XFA_Element eType = CXFA_Node::NameToElement(wsTagName);
        if (eType == XFA_Element::Unknown)
          continue;

        if (pXFANode->HasPropertyFlags(
                eType,
                XFA_PROPERTYFLAG_OneOf | XFA_PROPERTYFLAG_DefaultOneOf)) {
          if (bOneOfPropertyFound)
            break;
          bOneOfPropertyFound = true;
        }

        CXFA_Node* pXFAChild = m_pFactory->CreateNode(ePacketID, eType);
        if (!pXFAChild)
          return nullptr;
        if (ePacketID == XFA_PacketType::Config) {
          pXFAChild->JSObject()->SetAttribute(XFA_Attribute::Name,
                                              wsTagName.AsStringView(), false);
        }

        bool IsNeedValue = true;
        for (auto it : pXMLElement->GetAttributes()) {
          WideString wsAttrName;
          GetAttributeLocalName(it.first.AsStringView(), wsAttrName);
          if (wsAttrName == L"nil" && it.second == L"true")
            IsNeedValue = false;

          XFA_Attribute attr =
              CXFA_Node::NameToAttribute(wsAttrName.AsStringView());
          if (attr == XFA_Attribute::Unknown)
            continue;

          if (!bUseAttribute && attr != XFA_Attribute::Name &&
              attr != XFA_Attribute::Save) {
            continue;
          }
          pXFAChild->JSObject()->SetAttribute(attr, it.second.AsStringView(),
                                              false);
        }
        pXFANode->InsertChild(pXFAChild, nullptr);
        if (eType == XFA_Element::Validate || eType == XFA_Element::Locale) {
          if (ePacketID == XFA_PacketType::Config)
            ParseContentNode(pXFAChild, pXMLElement, ePacketID);
          else
            NormalLoader(pXFAChild, pXMLElement, ePacketID, bUseAttribute);

          break;
        }
        switch (pXFAChild->GetObjectType()) {
          case XFA_ObjectType::ContentNode:
          case XFA_ObjectType::TextNode:
          case XFA_ObjectType::NodeC:
          case XFA_ObjectType::NodeV:
            if (IsNeedValue)
              ParseContentNode(pXFAChild, pXMLElement, ePacketID);
            break;
          default:
            NormalLoader(pXFAChild, pXMLElement, ePacketID, bUseAttribute);
            break;
        }
      } break;
      case FX_XMLNODE_Instruction:
        ParseInstruction(pXFANode, static_cast<CFX_XMLInstruction*>(pXMLChild),
                         ePacketID);
        break;
      default:
        break;
    }
  }
  return pXFANode;
}

void CXFA_DocumentParser::ParseContentNode(CXFA_Node* pXFANode,
                                           CFX_XMLNode* pXMLNode,
                                           XFA_PacketType ePacketID) {
  XFA_Element element = XFA_Element::Sharptext;
  if (pXFANode->GetElementType() == XFA_Element::ExData) {
    WideString wsContentType =
        pXFANode->JSObject()->GetCData(XFA_Attribute::ContentType);
    if (wsContentType == L"text/html")
      element = XFA_Element::SharpxHTML;
    else if (wsContentType == L"text/xml")
      element = XFA_Element::Sharpxml;
  }
  if (element == XFA_Element::SharpxHTML)
    pXFANode->SetXMLMappingNode(pXMLNode);

  WideString wsValue;
  for (CFX_XMLNode* pXMLChild = pXMLNode->GetFirstChild(); pXMLChild;
       pXMLChild = pXMLChild->GetNextSibling()) {
    FX_XMLNODETYPE eNodeType = pXMLChild->GetType();
    if (eNodeType == FX_XMLNODE_Instruction)
      continue;

    if (element == XFA_Element::SharpxHTML) {
      if (eNodeType != FX_XMLNODE_Element)
        break;

      if (XFA_RecognizeRichText(static_cast<CFX_XMLElement*>(pXMLChild)))
        wsValue +=
            GetPlainTextFromRichText(static_cast<CFX_XMLElement*>(pXMLChild));
    } else if (element == XFA_Element::Sharpxml) {
      if (eNodeType != FX_XMLNODE_Element)
        break;

      ConvertXMLToPlainText(static_cast<CFX_XMLElement*>(pXMLChild), wsValue);
    } else {
      if (eNodeType == FX_XMLNODE_Element)
        break;
      if (eNodeType == FX_XMLNODE_Text || eNodeType == FX_XMLNODE_CharData)
        wsValue = static_cast<CFX_XMLText*>(pXMLChild)->GetText();
    }
    break;
  }
  if (!wsValue.IsEmpty()) {
    if (pXFANode->IsContentNode()) {
      CXFA_Node* pContentRawDataNode =
          m_pFactory->CreateNode(ePacketID, element);
      ASSERT(pContentRawDataNode);
      pContentRawDataNode->JSObject()->SetCData(XFA_Attribute::Value, wsValue,
                                                false, false);
      pXFANode->InsertChild(pContentRawDataNode, nullptr);
    } else {
      pXFANode->JSObject()->SetCData(XFA_Attribute::Value, wsValue, false,
                                     false);
    }
  }
}

void CXFA_DocumentParser::ParseDataGroup(CXFA_Node* pXFANode,
                                         CFX_XMLNode* pXMLNode,
                                         XFA_PacketType ePacketID) {
  for (CFX_XMLNode* pXMLChild = pXMLNode->GetFirstChild(); pXMLChild;
       pXMLChild = pXMLChild->GetNextSibling()) {
    switch (pXMLChild->GetType()) {
      case FX_XMLNODE_Element: {
        CFX_XMLElement* pXMLElement = static_cast<CFX_XMLElement*>(pXMLChild);
        {
          WideString wsNamespaceURI = pXMLElement->GetNamespaceURI();
          if (wsNamespaceURI == L"http://www.xfa.com/schema/xfa-package/" ||
              wsNamespaceURI == L"http://www.xfa.org/schema/xfa-package/" ||
              wsNamespaceURI == L"http://www.w3.org/2001/XMLSchema-instance") {
            continue;
          }
        }

        XFA_Element eNodeType = XFA_Element::DataModel;
        if (eNodeType == XFA_Element::DataModel) {
          WideString wsDataNodeAttr;
          if (FindAttributeWithNS(pXMLElement, L"dataNode",
                                  L"http://www.xfa.org/schema/xfa-data/1.0/",
                                  wsDataNodeAttr)) {
            if (wsDataNodeAttr == L"dataGroup")
              eNodeType = XFA_Element::DataGroup;
            else if (wsDataNodeAttr == L"dataValue")
              eNodeType = XFA_Element::DataValue;
          }
        }
        WideString wsContentType;
        if (eNodeType == XFA_Element::DataModel) {
          if (FindAttributeWithNS(pXMLElement, L"contentType",
                                  L"http://www.xfa.org/schema/xfa-data/1.0/",
                                  wsContentType)) {
            if (!wsContentType.IsEmpty())
              eNodeType = XFA_Element::DataValue;
          }
        }
        if (eNodeType == XFA_Element::DataModel) {
          for (CFX_XMLNode* pXMLDataChild = pXMLElement->GetFirstChild();
               pXMLDataChild; pXMLDataChild = pXMLDataChild->GetNextSibling()) {
            if (pXMLDataChild->GetType() == FX_XMLNODE_Element) {
              if (!XFA_RecognizeRichText(
                      static_cast<CFX_XMLElement*>(pXMLDataChild))) {
                eNodeType = XFA_Element::DataGroup;
                break;
              }
            }
          }
        }
        if (eNodeType == XFA_Element::DataModel)
          eNodeType = XFA_Element::DataValue;

        CXFA_Node* pXFAChild =
            m_pFactory->CreateNode(XFA_PacketType::Datasets, eNodeType);
        if (!pXFAChild)
          return;

        pXFAChild->JSObject()->SetCData(
            XFA_Attribute::Name, pXMLElement->GetLocalTagName(), false, false);
        bool bNeedValue = true;

        for (auto it : pXMLElement->GetAttributes()) {
          WideString wsName;
          WideString wsNS;
          if (!ResolveAttribute(pXMLElement, it.first, wsName, wsNS)) {
            continue;
          }
          if (wsName == L"nil" && it.second == L"true") {
            bNeedValue = false;
            continue;
          }
          if (wsNS == L"http://www.xfa.com/schema/xfa-package/" ||
              wsNS == L"http://www.xfa.org/schema/xfa-package/" ||
              wsNS == L"http://www.w3.org/2001/XMLSchema-instance" ||
              wsNS == L"http://www.xfa.org/schema/xfa-data/1.0/") {
            continue;
          }
          CXFA_Node* pXFAMetaData = m_pFactory->CreateNode(
              XFA_PacketType::Datasets, XFA_Element::DataValue);
          if (!pXFAMetaData)
            return;

          pXFAMetaData->JSObject()->SetCData(XFA_Attribute::Name, wsName, false,
                                             false);
          pXFAMetaData->JSObject()->SetCData(XFA_Attribute::QualifiedName,
                                             it.first, false, false);
          pXFAMetaData->JSObject()->SetCData(XFA_Attribute::Value, it.second,
                                             false, false);
          pXFAMetaData->JSObject()->SetEnum(XFA_Attribute::Contains,
                                            XFA_AttributeEnum::MetaData, false);
          pXFAChild->InsertChild(pXFAMetaData, nullptr);
          pXFAMetaData->SetXMLMappingNode(pXMLElement);
          pXFAMetaData->SetFlag(XFA_NodeFlag_Initialized);
        }

        if (!bNeedValue) {
          WideString wsNilName(L"xsi:nil");
          pXMLElement->RemoveAttribute(wsNilName.c_str());
        }
        pXFANode->InsertChild(pXFAChild, nullptr);
        if (eNodeType == XFA_Element::DataGroup)
          ParseDataGroup(pXFAChild, pXMLElement, ePacketID);
        else if (bNeedValue)
          ParseDataValue(pXFAChild, pXMLChild, XFA_PacketType::Datasets);

        pXFAChild->SetXMLMappingNode(pXMLElement);
        pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
        continue;
      }
      case FX_XMLNODE_CharData:
      case FX_XMLNODE_Text: {
        CFX_XMLText* pXMLText = static_cast<CFX_XMLText*>(pXMLChild);
        WideString wsText = pXMLText->GetText();
        if (IsStringAllWhitespace(wsText))
          continue;

        CXFA_Node* pXFAChild = m_pFactory->CreateNode(XFA_PacketType::Datasets,
                                                      XFA_Element::DataValue);
        if (!pXFAChild)
          return;

        pXFAChild->JSObject()->SetCData(XFA_Attribute::Value, wsText, false,
                                        false);
        pXFANode->InsertChild(pXFAChild, nullptr);
        pXFAChild->SetXMLMappingNode(pXMLText);
        pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
        continue;
      }
      default:
        continue;
    }
  }
}

void CXFA_DocumentParser::ParseDataValue(CXFA_Node* pXFANode,
                                         CFX_XMLNode* pXMLNode,
                                         XFA_PacketType ePacketID) {
  CFX_WideTextBuf wsValueTextBuf;
  CFX_WideTextBuf wsCurValueTextBuf;
  bool bMarkAsCompound = false;
  CFX_XMLNode* pXMLCurValueNode = nullptr;
  for (CFX_XMLNode* pXMLChild = pXMLNode->GetFirstChild(); pXMLChild;
       pXMLChild = pXMLChild->GetNextSibling()) {
    FX_XMLNODETYPE eNodeType = pXMLChild->GetType();
    if (eNodeType == FX_XMLNODE_Instruction)
      continue;

    if (eNodeType == FX_XMLNODE_Text || eNodeType == FX_XMLNODE_CharData) {
      WideString wsText = static_cast<CFX_XMLText*>(pXMLChild)->GetText();
      if (!pXMLCurValueNode)
        pXMLCurValueNode = pXMLChild;

      wsCurValueTextBuf << wsText;
    } else if (XFA_RecognizeRichText(static_cast<CFX_XMLElement*>(pXMLChild))) {
      WideString wsText =
          GetPlainTextFromRichText(static_cast<CFX_XMLElement*>(pXMLChild));
      if (!pXMLCurValueNode)
        pXMLCurValueNode = pXMLChild;

      wsCurValueTextBuf << wsText;
    } else {
      bMarkAsCompound = true;
      if (pXMLCurValueNode) {
        WideString wsCurValue = wsCurValueTextBuf.MakeString();
        if (!wsCurValue.IsEmpty()) {
          CXFA_Node* pXFAChild =
              m_pFactory->CreateNode(ePacketID, XFA_Element::DataValue);
          if (!pXFAChild)
            return;

          pXFAChild->JSObject()->SetCData(XFA_Attribute::Name, L"", false,
                                          false);
          pXFAChild->JSObject()->SetCData(XFA_Attribute::Value, wsCurValue,
                                          false, false);
          pXFANode->InsertChild(pXFAChild, nullptr);
          pXFAChild->SetXMLMappingNode(pXMLCurValueNode);
          pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
          wsValueTextBuf << wsCurValue;
          wsCurValueTextBuf.Clear();
        }
        pXMLCurValueNode = nullptr;
      }
      CXFA_Node* pXFAChild =
          m_pFactory->CreateNode(ePacketID, XFA_Element::DataValue);
      if (!pXFAChild)
        return;

      WideString wsNodeStr =
          static_cast<CFX_XMLElement*>(pXMLChild)->GetLocalTagName();
      pXFAChild->JSObject()->SetCData(XFA_Attribute::Name, wsNodeStr, false,
                                      false);
      ParseDataValue(pXFAChild, pXMLChild, ePacketID);
      pXFANode->InsertChild(pXFAChild, nullptr);
      pXFAChild->SetXMLMappingNode(pXMLChild);
      pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
      WideString wsCurValue =
          pXFAChild->JSObject()->GetCData(XFA_Attribute::Value);
      wsValueTextBuf << wsCurValue;
    }
  }
  if (pXMLCurValueNode) {
    WideString wsCurValue = wsCurValueTextBuf.MakeString();
    if (!wsCurValue.IsEmpty()) {
      if (bMarkAsCompound) {
        CXFA_Node* pXFAChild =
            m_pFactory->CreateNode(ePacketID, XFA_Element::DataValue);
        if (!pXFAChild)
          return;

        pXFAChild->JSObject()->SetCData(XFA_Attribute::Name, L"", false, false);
        pXFAChild->JSObject()->SetCData(XFA_Attribute::Value, wsCurValue, false,
                                        false);
        pXFANode->InsertChild(pXFAChild, nullptr);
        pXFAChild->SetXMLMappingNode(pXMLCurValueNode);
        pXFAChild->SetFlag(XFA_NodeFlag_Initialized);
      }
      wsValueTextBuf << wsCurValue;
      wsCurValueTextBuf.Clear();
    }
    pXMLCurValueNode = nullptr;
  }
  WideString wsNodeValue = wsValueTextBuf.MakeString();
  pXFANode->JSObject()->SetCData(XFA_Attribute::Value, wsNodeValue, false,
                                 false);
}

void CXFA_DocumentParser::ParseInstruction(CXFA_Node* pXFANode,
                                           CFX_XMLInstruction* pXMLInstruction,
                                           XFA_PacketType ePacketID) {
  const std::vector<WideString>& target_data = pXMLInstruction->GetTargetData();

  if (pXMLInstruction->IsOriginalXFAVersion()) {
    if (target_data.size() > 1 &&
        (pXFANode->GetDocument()->RecognizeXFAVersionNumber(target_data[0]) !=
         XFA_VERSION_UNKNOWN) &&
        target_data[1] == L"v2.7-scripting:1") {
      pXFANode->GetDocument()->SetFlag(XFA_DOCFLAG_Scripting, true);
    }
    return;
  }

  if (pXMLInstruction->IsAcrobat()) {
    if (target_data.size() > 1 && target_data[0] == L"JavaScript" &&
        target_data[1] == L"strictScoping") {
      pXFANode->GetDocument()->SetFlag(XFA_DOCFLAG_StrictScoping, true);
    }
  }
}
