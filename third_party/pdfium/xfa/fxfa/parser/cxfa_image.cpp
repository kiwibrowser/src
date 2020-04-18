// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_image.h"

#include "fxjs/xfa/cjx_image.h"
#include "third_party/base/ptr_util.h"

namespace {

const CXFA_Node::AttributeData kImageAttributeData[] = {
    {XFA_Attribute::Id, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Name, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Use, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::ContentType, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::TransferEncoding, XFA_AttributeType::Enum,
     (void*)XFA_AttributeEnum::Base64},
    {XFA_Attribute::Usehref, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Aspect, XFA_AttributeType::Enum,
     (void*)XFA_AttributeEnum::Fit},
    {XFA_Attribute::Href, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Unknown, XFA_AttributeType::Integer, nullptr}};

constexpr wchar_t kImageName[] = L"image";

}  // namespace

CXFA_Image::CXFA_Image(CXFA_Document* doc, XFA_PacketType packet)
    : CXFA_Node(doc,
                packet,
                (XFA_XDPPACKET_Template | XFA_XDPPACKET_Form),
                XFA_ObjectType::ContentNode,
                XFA_Element::Image,
                nullptr,
                kImageAttributeData,
                kImageName) {}

CXFA_Image::~CXFA_Image() {}

XFA_AttributeEnum CXFA_Image::GetAspect() {
  return JSObject()->GetEnum(XFA_Attribute::Aspect);
}

WideString CXFA_Image::GetContentType() {
  return JSObject()->TryCData(XFA_Attribute::ContentType, true).value_or(L"");
}

WideString CXFA_Image::GetHref() {
  return JSObject()->TryCData(XFA_Attribute::Href, true).value_or(L"");
}

XFA_AttributeEnum CXFA_Image::GetTransferEncoding() {
  return static_cast<XFA_AttributeEnum>(
      JSObject()->GetEnum(XFA_Attribute::TransferEncoding));
}

WideString CXFA_Image::GetContent() {
  return JSObject()->TryContent(false, true).value_or(L"");
}

void CXFA_Image::SetContentType(const WideString& wsContentType) {
  JSObject()->SetCData(XFA_Attribute::ContentType, wsContentType, false, false);
}

void CXFA_Image::SetHref(const WideString& wsHref) {
  JSObject()->SetCData(XFA_Attribute::Href, wsHref, false, false);
}

void CXFA_Image::SetTransferEncoding(XFA_AttributeEnum iTransferEncoding) {
  JSObject()->SetEnum(XFA_Attribute::TransferEncoding, iTransferEncoding,
                      false);
}
