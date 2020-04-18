/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/html/html_hr_element.h"

#include "third_party/blink/renderer/core/css/css_color_value.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/html/forms/html_opt_group_element.h"
#include "third_party/blink/renderer/core/html/forms/html_select_element.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

using namespace cssvalue;
using namespace HTMLNames;

inline HTMLHRElement::HTMLHRElement(Document& document)
    : HTMLElement(hrTag, document) {}

DEFINE_NODE_FACTORY(HTMLHRElement)

bool HTMLHRElement::IsPresentationAttribute(const QualifiedName& name) const {
  if (name == alignAttr || name == widthAttr || name == colorAttr ||
      name == noshadeAttr || name == sizeAttr)
    return true;
  return HTMLElement::IsPresentationAttribute(name);
}

void HTMLHRElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableCSSPropertyValueSet* style) {
  if (name == alignAttr) {
    if (DeprecatedEqualIgnoringCase(value, "left")) {
      AddPropertyToPresentationAttributeStyle(
          style, CSSPropertyMarginLeft, 0,
          CSSPrimitiveValue::UnitType::kPixels);
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyMarginRight,
                                              CSSValueAuto);
    } else if (DeprecatedEqualIgnoringCase(value, "right")) {
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyMarginLeft,
                                              CSSValueAuto);
      AddPropertyToPresentationAttributeStyle(
          style, CSSPropertyMarginRight, 0,
          CSSPrimitiveValue::UnitType::kPixels);
    } else {
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyMarginLeft,
                                              CSSValueAuto);
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyMarginRight,
                                              CSSValueAuto);
    }
  } else if (name == widthAttr) {
    bool ok;
    int v = value.ToInt(&ok);
    if (ok && !v)
      AddPropertyToPresentationAttributeStyle(
          style, CSSPropertyWidth, 1, CSSPrimitiveValue::UnitType::kPixels);
    else
      AddHTMLLengthToStyle(style, CSSPropertyWidth, value);
  } else if (name == colorAttr) {
    AddPropertyToPresentationAttributeStyle(style, CSSPropertyBorderStyle,
                                            CSSValueSolid);
    AddHTMLColorToStyle(style, CSSPropertyBorderColor, value);
    AddHTMLColorToStyle(style, CSSPropertyBackgroundColor, value);
  } else if (name == noshadeAttr) {
    if (!hasAttribute(colorAttr)) {
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyBorderStyle,
                                              CSSValueSolid);

      const CSSColorValue& dark_gray_value =
          *CSSColorValue::Create(Color::kDarkGray);
      style->SetProperty(CSSPropertyBorderColor, dark_gray_value);
      style->SetProperty(CSSPropertyBackgroundColor, dark_gray_value);
    }
  } else if (name == sizeAttr) {
    int size = value.ToInt();
    if (size <= 1)
      AddPropertyToPresentationAttributeStyle(
          style, CSSPropertyBorderBottomWidth, 0,
          CSSPrimitiveValue::UnitType::kPixels);
    else
      AddPropertyToPresentationAttributeStyle(
          style, CSSPropertyHeight, size - 2,
          CSSPrimitiveValue::UnitType::kPixels);
  } else {
    HTMLElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

HTMLSelectElement* HTMLHRElement::OwnerSelectElement() const {
  if (!parentNode())
    return nullptr;
  if (auto* select = ToHTMLSelectElementOrNull(*parentNode()))
    return select;
  if (!IsHTMLOptGroupElement(*parentNode()))
    return nullptr;
  return ToHTMLSelectElementOrNull(parentNode()->parentNode());
}

Node::InsertionNotificationRequest HTMLHRElement::InsertedInto(
    ContainerNode* insertion_point) {
  HTMLElement::InsertedInto(insertion_point);
  if (HTMLSelectElement* select = OwnerSelectElement()) {
    if (insertion_point == select || (IsHTMLOptGroupElement(*insertion_point) &&
                                      insertion_point->parentNode() == select))
      select->HrInsertedOrRemoved(*this);
  }
  return kInsertionDone;
}

void HTMLHRElement::RemovedFrom(ContainerNode* insertion_point) {
  if (auto* select = ToHTMLSelectElementOrNull(*insertion_point)) {
    if (!parentNode() || IsHTMLOptGroupElement(*parentNode()))
      select->HrInsertedOrRemoved(*this);
  } else if (IsHTMLOptGroupElement(*insertion_point)) {
    Node* parent = insertion_point->parentNode();
    if (auto* select = ToHTMLSelectElementOrNull(parent))
      select->HrInsertedOrRemoved(*this);
  }
  HTMLElement::RemovedFrom(insertion_point);
}

}  // namespace blink
