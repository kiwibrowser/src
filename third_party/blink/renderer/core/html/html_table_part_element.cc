/**
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
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
 */

#include "third_party/blink/renderer/core/html/html_table_part_element.h"

#include "third_party/blink/renderer/core/css/css_image_value.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/flat_tree_traversal.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"

namespace blink {

using namespace HTMLNames;

bool HTMLTablePartElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == bgcolorAttr || name == backgroundAttr || name == valignAttr ||
      name == alignAttr || name == heightAttr)
    return true;
  return HTMLElement::IsPresentationAttribute(name);
}

void HTMLTablePartElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableCSSPropertyValueSet* style) {
  if (name == bgcolorAttr) {
    AddHTMLColorToStyle(style, CSSPropertyBackgroundColor, value);
  } else if (name == backgroundAttr) {
    String url = StripLeadingAndTrailingHTMLSpaces(value);
    if (!url.IsEmpty()) {
      UseCounter::Count(
          GetDocument(),
          WebFeature::kHTMLTableElementPresentationAttributeBackground);
      CSSImageValue* image_value =
          CSSImageValue::Create(url, GetDocument().CompleteURL(url),
                                Referrer(GetDocument().OutgoingReferrer(),
                                         GetDocument().GetReferrerPolicy()));
      style->SetProperty(
          CSSPropertyValue(GetCSSPropertyBackgroundImage(), *image_value));
    }
  } else if (name == valignAttr) {
    if (DeprecatedEqualIgnoringCase(value, "top"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign,
                                              CSSValueTop);
    else if (DeprecatedEqualIgnoringCase(value, "middle"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign,
                                              CSSValueMiddle);
    else if (DeprecatedEqualIgnoringCase(value, "bottom"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign,
                                              CSSValueBottom);
    else if (DeprecatedEqualIgnoringCase(value, "baseline"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign,
                                              CSSValueBaseline);
    else
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign,
                                              value);
  } else if (name == alignAttr) {
    if (DeprecatedEqualIgnoringCase(value, "middle") ||
        DeprecatedEqualIgnoringCase(value, "center"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign,
                                              CSSValueWebkitCenter);
    else if (DeprecatedEqualIgnoringCase(value, "absmiddle"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign,
                                              CSSValueCenter);
    else if (DeprecatedEqualIgnoringCase(value, "left"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign,
                                              CSSValueWebkitLeft);
    else if (DeprecatedEqualIgnoringCase(value, "right"))
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign,
                                              CSSValueWebkitRight);
    else
      AddPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign,
                                              value);
  } else if (name == heightAttr) {
    if (!value.IsEmpty())
      AddHTMLLengthToStyle(style, CSSPropertyHeight, value);
  } else {
    HTMLElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

HTMLTableElement* HTMLTablePartElement::FindParentTable() const {
  ContainerNode* parent = FlatTreeTraversal::Parent(*this);
  while (parent && !IsHTMLTableElement(*parent))
    parent = FlatTreeTraversal::Parent(*parent);
  return ToHTMLTableElement(parent);
}

}  // namespace blink
