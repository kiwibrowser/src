/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/html_table_cell_element.h"

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/dom/attribute.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html/table_constants.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"

namespace blink {

using namespace HTMLNames;

inline HTMLTableCellElement::HTMLTableCellElement(const QualifiedName& tag_name,
                                                  Document& document)
    : HTMLTablePartElement(tag_name, document) {}

DEFINE_ELEMENT_FACTORY_WITH_TAGNAME(HTMLTableCellElement)

unsigned HTMLTableCellElement::colSpan() const {
  const AtomicString& col_span_value = FastGetAttribute(colspanAttr);
  unsigned value = 0;
  if (!ParseHTMLClampedNonNegativeInteger(col_span_value, kMinColSpan,
                                          kMaxColSpan, value))
    return kDefaultColSpan;
  // Counting for https://github.com/whatwg/html/issues/1198
  UseCounter::Count(GetDocument(), WebFeature::kHTMLTableCellElementColspan);
  if (value > 8190) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kHTMLTableCellElementColspanGreaterThan8190);
  } else if (value > 1000) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kHTMLTableCellElementColspanGreaterThan1000);
  }
  return value;
}

unsigned HTMLTableCellElement::rowSpan() const {
  const AtomicString& row_span_value = FastGetAttribute(rowspanAttr);
  unsigned value = 0;
  if (!ParseHTMLClampedNonNegativeInteger(row_span_value, kMinRowSpan,
                                          kMaxRowSpan, value))
    return kDefaultRowSpan;
  return value;
}

int HTMLTableCellElement::cellIndex() const {
  if (!IsHTMLTableRowElement(parentElement()))
    return -1;

  int index = 0;
  for (const HTMLTableCellElement* element =
           Traversal<HTMLTableCellElement>::PreviousSibling(*this);
       element;
       element = Traversal<HTMLTableCellElement>::PreviousSibling(*element))
    ++index;

  return index;
}

bool HTMLTableCellElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == nowrapAttr || name == widthAttr || name == heightAttr)
    return true;
  return HTMLTablePartElement::IsPresentationAttribute(name);
}

void HTMLTableCellElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableCSSPropertyValueSet* style) {
  if (name == nowrapAttr) {
    AddPropertyToPresentationAttributeStyle(style, CSSPropertyWhiteSpace,
                                            CSSValueWebkitNowrap);
  } else if (name == widthAttr) {
    if (!value.IsEmpty()) {
      int width_int = value.ToInt();
      if (width_int > 0)  // width="0" is ignored for compatibility with WinIE.
        AddHTMLLengthToStyle(style, CSSPropertyWidth, value);
    }
  } else if (name == heightAttr) {
    if (!value.IsEmpty()) {
      int height_int = value.ToInt();
      if (height_int >
          0)  // height="0" is ignored for compatibility with WinIE.
        AddHTMLLengthToStyle(style, CSSPropertyHeight, value);
    }
  } else {
    HTMLTablePartElement::CollectStyleForPresentationAttribute(name, value,
                                                               style);
  }
}

void HTMLTableCellElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == rowspanAttr || params.name == colspanAttr) {
    if (GetLayoutObject() && GetLayoutObject()->IsTableCell())
      ToLayoutTableCell(GetLayoutObject())->ColSpanOrRowSpanChanged();
  } else {
    HTMLTablePartElement::ParseAttribute(params);
  }
}

const CSSPropertyValueSet*
HTMLTableCellElement::AdditionalPresentationAttributeStyle() {
  if (HTMLTableElement* table = FindParentTable())
    return table->AdditionalCellStyle();
  return nullptr;
}

bool HTMLTableCellElement::IsURLAttribute(const Attribute& attribute) const {
  return attribute.GetName() == backgroundAttr ||
         HTMLTablePartElement::IsURLAttribute(attribute);
}

bool HTMLTableCellElement::HasLegalLinkAttribute(
    const QualifiedName& name) const {
  return (HasTagName(tdTag) && name == backgroundAttr) ||
         HTMLTablePartElement::HasLegalLinkAttribute(name);
}

const QualifiedName& HTMLTableCellElement::SubResourceAttributeName() const {
  return HasTagName(tdTag) ? backgroundAttr
                           : HTMLTablePartElement::SubResourceAttributeName();
}

const AtomicString& HTMLTableCellElement::Abbr() const {
  return FastGetAttribute(abbrAttr);
}

const AtomicString& HTMLTableCellElement::Axis() const {
  return FastGetAttribute(axisAttr);
}

void HTMLTableCellElement::setColSpan(unsigned n) {
  SetUnsignedIntegralAttribute(colspanAttr, n, kDefaultColSpan);
}

const AtomicString& HTMLTableCellElement::Headers() const {
  return FastGetAttribute(headersAttr);
}

void HTMLTableCellElement::setRowSpan(unsigned n) {
  SetUnsignedIntegralAttribute(rowspanAttr, n, kDefaultRowSpan);
}

}  // namespace blink
