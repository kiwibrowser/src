/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.
 * All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FIRST_LETTER_PSEUDO_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FIRST_LETTER_PSEUDO_ELEMENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Element;
class LayoutObject;
class LayoutTextFragment;

class CORE_EXPORT FirstLetterPseudoElement final : public PseudoElement {
 public:
  static FirstLetterPseudoElement* Create(Element* parent) {
    return new FirstLetterPseudoElement(parent);
  }

  ~FirstLetterPseudoElement() override;

  static LayoutObject* FirstLetterTextLayoutObject(const Element&);
  static unsigned FirstLetterLength(const String&);

  void SetRemainingTextLayoutObject(LayoutTextFragment*);
  LayoutTextFragment* RemainingTextLayoutObject() const {
    return remaining_text_layout_object_;
  }

  void UpdateTextFragments();

  void AttachLayoutTree(AttachContext&) override;
  void DetachLayoutTree(const AttachContext& = AttachContext()) override;

 private:
  explicit FirstLetterPseudoElement(Element*);

  void DidRecalcStyle(StyleRecalcChange) override;

  void AttachFirstLetterTextLayoutObjects();
  ComputedStyle* StyleForFirstLetter(LayoutObject*);

  LayoutTextFragment* remaining_text_layout_object_;
  DISALLOW_COPY_AND_ASSIGN(FirstLetterPseudoElement);
};

DEFINE_ELEMENT_TYPE_CASTS(FirstLetterPseudoElement,
                          IsFirstLetterPseudoElement());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_FIRST_LETTER_PSEUDO_ELEMENT_H_
