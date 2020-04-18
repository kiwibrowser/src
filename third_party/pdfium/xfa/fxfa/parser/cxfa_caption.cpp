// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_caption.h"

#include "fxjs/xfa/cjx_caption.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/parser/cxfa_font.h"
#include "xfa/fxfa/parser/cxfa_margin.h"
#include "xfa/fxfa/parser/cxfa_measurement.h"
#include "xfa/fxfa/parser/cxfa_value.h"

namespace {

const CXFA_Node::PropertyData kCaptionPropertyData[] = {
    {XFA_Element::Margin, 1, 0}, {XFA_Element::Para, 1, 0},
    {XFA_Element::Font, 1, 0},   {XFA_Element::Value, 1, 0},
    {XFA_Element::Extras, 1, 0}, {XFA_Element::Unknown, 0, 0}};
const CXFA_Node::AttributeData kCaptionAttributeData[] = {
    {XFA_Attribute::Id, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Use, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Reserve, XFA_AttributeType::Measure, (void*)L"-1un"},
    {XFA_Attribute::Presence, XFA_AttributeType::Enum,
     (void*)XFA_AttributeEnum::Visible},
    {XFA_Attribute::Usehref, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Placement, XFA_AttributeType::Enum,
     (void*)XFA_AttributeEnum::Left},
    {XFA_Attribute::Unknown, XFA_AttributeType::Integer, nullptr}};

constexpr wchar_t kCaptionName[] = L"caption";

}  // namespace

CXFA_Caption::CXFA_Caption(CXFA_Document* doc, XFA_PacketType packet)
    : CXFA_Node(doc,
                packet,
                (XFA_XDPPACKET_Template | XFA_XDPPACKET_Form),
                XFA_ObjectType::Node,
                XFA_Element::Caption,
                kCaptionPropertyData,
                kCaptionAttributeData,
                kCaptionName,
                pdfium::MakeUnique<CJX_Caption>(this)) {}

CXFA_Caption::~CXFA_Caption() {}

bool CXFA_Caption::IsVisible() {
  return JSObject()
             ->TryEnum(XFA_Attribute::Presence, true)
             .value_or(XFA_AttributeEnum::Visible) ==
         XFA_AttributeEnum::Visible;
}

bool CXFA_Caption::IsHidden() {
  return JSObject()
             ->TryEnum(XFA_Attribute::Presence, true)
             .value_or(XFA_AttributeEnum::Visible) == XFA_AttributeEnum::Hidden;
}

XFA_AttributeEnum CXFA_Caption::GetPlacementType() {
  return JSObject()
      ->TryEnum(XFA_Attribute::Placement, true)
      .value_or(XFA_AttributeEnum::Left);
}

float CXFA_Caption::GetReserve() const {
  return JSObject()->GetMeasure(XFA_Attribute::Reserve).ToUnit(XFA_Unit::Pt);
}

CXFA_Margin* CXFA_Caption::GetMarginIfExists() {
  return GetChild<CXFA_Margin>(0, XFA_Element::Margin, false);
}

CXFA_Font* CXFA_Caption::GetFontIfExists() {
  return GetChild<CXFA_Font>(0, XFA_Element::Font, false);
}

CXFA_Value* CXFA_Caption::GetValueIfExists() {
  return GetChild<CXFA_Value>(0, XFA_Element::Value, false);
}
