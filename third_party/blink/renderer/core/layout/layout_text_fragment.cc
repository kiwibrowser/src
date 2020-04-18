/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"

#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"

namespace blink {

LayoutTextFragment::LayoutTextFragment(Node* node,
                                       StringImpl* str,
                                       int start_offset,
                                       int length)
    : LayoutText(node, str ? str->Substring(start_offset, length) : nullptr),
      start_(start_offset),
      fragment_length_(length),
      is_remaining_text_layout_object_(false),
      content_string_(str),
      first_letter_pseudo_element_(nullptr) {}

LayoutTextFragment::LayoutTextFragment(Node* node, StringImpl* str)
    : LayoutTextFragment(node, str, 0, str ? str->length() : 0) {}

LayoutTextFragment::~LayoutTextFragment() {
  DCHECK(!first_letter_pseudo_element_);
}

LayoutTextFragment* LayoutTextFragment::CreateAnonymous(PseudoElement& pseudo,
                                                        StringImpl* text,
                                                        unsigned start,
                                                        unsigned length) {
  LayoutTextFragment* fragment =
      new LayoutTextFragment(nullptr, text, start, length);
  fragment->SetDocumentForAnonymous(&pseudo.GetDocument());
  if (length)
    pseudo.GetDocument().View()->IncrementVisuallyNonEmptyCharacterCount(
        length);
  return fragment;
}

LayoutTextFragment* LayoutTextFragment::CreateAnonymous(PseudoElement& pseudo,
                                                        StringImpl* text) {
  return CreateAnonymous(pseudo, text, 0, text ? text->length() : 0);
}

void LayoutTextFragment::WillBeDestroyed() {
  if (is_remaining_text_layout_object_ && first_letter_pseudo_element_)
    first_letter_pseudo_element_->SetRemainingTextLayoutObject(nullptr);
  first_letter_pseudo_element_ = nullptr;
  LayoutText::WillBeDestroyed();
}

scoped_refptr<StringImpl> LayoutTextFragment::CompleteText() const {
  Text* text = AssociatedTextNode();
  return text ? text->DataImpl() : ContentString();
}

void LayoutTextFragment::SetContentString(StringImpl* str) {
  content_string_ = str;
  SetText(str);
}

scoped_refptr<StringImpl> LayoutTextFragment::OriginalText() const {
  scoped_refptr<StringImpl> result = CompleteText();
  if (!result)
    return nullptr;
  return result->Substring(Start(), FragmentLength());
}

void LayoutTextFragment::SetText(scoped_refptr<StringImpl> text, bool force) {
  LayoutText::SetText(std::move(text), force);

  start_ = 0;
  fragment_length_ = TextLength();

  // If we're the remaining text from a first letter then we have to tell the
  // first letter pseudo element to reattach itself so it can re-calculate the
  // correct first-letter settings.
  if (IsRemainingTextLayoutObject()) {
    DCHECK(GetFirstLetterPseudoElement());
    GetFirstLetterPseudoElement()->UpdateTextFragments();
  }
}

void LayoutTextFragment::SetTextFragment(scoped_refptr<StringImpl> text,
                                         unsigned start,
                                         unsigned length) {
  LayoutText::SetText(std::move(text), false);

  start_ = start;
  fragment_length_ = length;
}

void LayoutTextFragment::TransformText() {
  // Note, we have to call LayoutText::setText here because, if we use our
  // version we will, potentially, screw up the first-letter settings where
  // we only use portions of the string.
  if (scoped_refptr<StringImpl> text_to_transform = OriginalText())
    LayoutText::SetText(std::move(text_to_transform), true);
}

UChar LayoutTextFragment::PreviousCharacter() const {
  if (Start()) {
    StringImpl* original = CompleteText().get();
    if (original && Start() <= original->length())
      return (*original)[Start() - 1];
  }

  return LayoutText::PreviousCharacter();
}

// If this is the layoutObject for a first-letter pseudoNode then we have to
// look at the node for the remaining text to find our content.
Text* LayoutTextFragment::AssociatedTextNode() const {
  Node* node = GetFirstLetterPseudoElement();
  if (is_remaining_text_layout_object_ || !node) {
    // If we don't have a node, then we aren't part of a first-letter pseudo
    // element, so use the actual node. Likewise, if we have a node, but
    // we're the remainingTextLayoutObject for a pseudo element use the real
    // text node.
    node = GetNode();
  }

  if (!node)
    return nullptr;

  if (node->IsFirstLetterPseudoElement()) {
    FirstLetterPseudoElement* pseudo = ToFirstLetterPseudoElement(node);
    LayoutObject* next_layout_object =
        FirstLetterPseudoElement::FirstLetterTextLayoutObject(*pseudo);
    if (!next_layout_object)
      return nullptr;
    node = next_layout_object->GetNode();
  }
  return (node && node->IsTextNode()) ? ToText(node) : nullptr;
}

void LayoutTextFragment::UpdateHitTestResult(HitTestResult& result,
                                             const LayoutPoint& point) {
  if (result.InnerNode())
    return;

  LayoutObject::UpdateHitTestResult(result, point);

  // If we aren't part of a first-letter element, or if we
  // are part of first-letter but we're the remaining text then return.
  if (is_remaining_text_layout_object_ || !GetFirstLetterPseudoElement())
    return;
  result.SetInnerNode(GetFirstLetterPseudoElement());
}

Position LayoutTextFragment::PositionForCaretOffset(unsigned offset) const {
  DCHECK_LE(offset, FragmentLength());
  const Text* node = AssociatedTextNode();
  if (!node)
    return Position();
  // TODO(layout-dev): Support offset change due to text-transform.
  return Position(node, Start() + offset);
}

base::Optional<unsigned> LayoutTextFragment::CaretOffsetForPosition(
    const Position& position) const {
  if (position.IsNull() || position.AnchorNode() != AssociatedTextNode())
    return base::nullopt;
  unsigned dom_offset;
  if (position.IsBeforeAnchor()) {
    dom_offset = 0;
  } else if (position.IsAfterAnchor()) {
    // TODO(layout-dev): Support offset change due to text-transform.
    dom_offset = Start() + FragmentLength();
  } else {
    DCHECK(position.IsOffsetInAnchor()) << position;
    // TODO(layout-dev): Support offset change due to text-transform.
    dom_offset = position.OffsetInContainerNode();
  }
  if (dom_offset < Start() || dom_offset > Start() + FragmentLength())
    return base::nullopt;
  return dom_offset - Start();
}

}  // namespace blink
