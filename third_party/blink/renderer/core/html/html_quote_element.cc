/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/html_quote_element.h"

#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

using namespace HTMLNames;

inline HTMLQuoteElement::HTMLQuoteElement(const QualifiedName& tag_name,
                                          Document& document)
    : HTMLElement(tag_name, document) {
  DCHECK(HasTagName(qTag) || HasTagName(blockquoteTag));
}

DEFINE_ELEMENT_FACTORY_WITH_TAGNAME(HTMLQuoteElement)

bool HTMLQuoteElement::IsURLAttribute(const Attribute& attribute) const {
  return attribute.GetName() == citeAttr ||
         HTMLElement::IsURLAttribute(attribute);
}

bool HTMLQuoteElement::HasLegalLinkAttribute(const QualifiedName& name) const {
  return name == citeAttr || HTMLElement::HasLegalLinkAttribute(name);
}

const QualifiedName& HTMLQuoteElement::SubResourceAttributeName() const {
  return citeAttr;
}

}  // namespace blink
