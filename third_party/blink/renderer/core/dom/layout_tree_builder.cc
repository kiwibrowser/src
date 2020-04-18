/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/dom/layout_tree_builder.h"

#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/dom/v0_insertion_point.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_full_screen.h"
#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg_names.h"

namespace blink {

LayoutTreeBuilderForElement::LayoutTreeBuilderForElement(Element& element,
                                                         ComputedStyle* style)
    : LayoutTreeBuilder(element, nullptr), style_(style) {
  DCHECK(element.CanParticipateInFlatTree());
  // TODO(ecobos): Move the first-letter logic inside ParentLayoutObject too?
  // It's an extra (unnecessary) check for text nodes, though.
  if (element.IsFirstLetterPseudoElement()) {
    if (LayoutObject* next_layout_object =
            FirstLetterPseudoElement::FirstLetterTextLayoutObject(element))
      layout_object_parent_ = next_layout_object->Parent();
  } else {
    layout_object_parent_ =
        LayoutTreeBuilderTraversal::ParentLayoutObject(element);
  }
}

LayoutObject* LayoutTreeBuilderForElement::NextLayoutObject() const {
  DCHECK(layout_object_parent_);

  if (node_->IsInTopLayer())
    return LayoutTreeBuilderTraversal::NextInTopLayer(*node_);

  if (node_->IsFirstLetterPseudoElement())
    return FirstLetterPseudoElement::FirstLetterTextLayoutObject(*node_);

  return LayoutTreeBuilder::NextLayoutObject();
}

LayoutObject* LayoutTreeBuilderForElement::ParentLayoutObject() const {
  if (layout_object_parent_) {
    // FIXME: Guarding this by ParentLayoutObject isn't quite right as the spec
    // for top layer only talks about display: none ancestors so putting a
    // <dialog> inside an <optgroup> seems like it should still work even though
    // this check will prevent it.
    if (node_->IsInTopLayer())
      return node_->GetDocument().GetLayoutView();
  }

  return layout_object_parent_;
}

DISABLE_CFI_PERF
bool LayoutTreeBuilderForElement::ShouldCreateLayoutObject() const {
  if (!layout_object_parent_)
    return false;

  LayoutObject* parent_layout_object = ParentLayoutObject();
  if (!parent_layout_object)
    return false;
  if (!parent_layout_object->CanHaveChildren())
    return false;
  return node_->LayoutObjectIsNeeded(Style());
}

ComputedStyle& LayoutTreeBuilderForElement::Style() const {
  if (!style_) {
    DCHECK(!node_->GetNonAttachedStyle());
    style_ = node_->StyleForLayoutObject();
  }
  return *style_;
}

DISABLE_CFI_PERF
void LayoutTreeBuilderForElement::CreateLayoutObject() {
  ComputedStyle& style = Style();

  LayoutObject* new_layout_object = node_->CreateLayoutObject(style);
  if (!new_layout_object)
    return;

  LayoutObject* parent_layout_object = ParentLayoutObject();

  if (!parent_layout_object->IsChildAllowed(new_layout_object, style)) {
    new_layout_object->Destroy();
    return;
  }

  // Make sure the LayoutObject already knows it is going to be added to a
  // LayoutFlowThread before we set the style for the first time. Otherwise code
  // using IsInsideFlowThread() in the StyleWillChange and StyleDidChange will
  // fail.
  new_layout_object->SetIsInsideFlowThread(
      parent_layout_object->IsInsideFlowThread());

  LayoutObject* next_layout_object = NextLayoutObject();
  node_->SetLayoutObject(new_layout_object);
  new_layout_object->SetStyle(
      &style);  // SetStyle() can depend on LayoutObject() already being set.

  Document& document = node_->GetDocument();
  if (Fullscreen::IsFullscreenElement(*node_) &&
      node_.Get() != document.documentElement()) {
    new_layout_object = LayoutFullScreen::WrapLayoutObject(
        new_layout_object, parent_layout_object, &document);
    if (!new_layout_object)
      return;
  }

  // Note: Adding new_layout_object instead of LayoutObject(). LayoutObject()
  // may be a child of new_layout_object.
  parent_layout_object->AddChild(new_layout_object, next_layout_object);
}

LayoutObject*
LayoutTreeBuilderForText::CreateInlineWrapperForDisplayContentsIfNeeded() {
  scoped_refptr<ComputedStyle> wrapper_style =
      ComputedStyle::CreateInheritedDisplayContentsStyleIfNeeded(
          *style_, layout_object_parent_->StyleRef());
  if (!wrapper_style)
    return nullptr;

  // Text nodes which are children of display:contents element which modifies
  // inherited properties, need to have an anonymous inline wrapper having those
  // inherited properties because the layout code expects the LayoutObject
  // parent of text nodes to have the same inherited properties.
  LayoutObject* inline_wrapper =
      LayoutInline::CreateAnonymous(&node_->GetDocument());
  inline_wrapper->SetStyle(wrapper_style);
  if (!layout_object_parent_->IsChildAllowed(inline_wrapper, *wrapper_style)) {
    inline_wrapper->Destroy();
    return nullptr;
  }
  layout_object_parent_->AddChild(inline_wrapper, NextLayoutObject());
  return inline_wrapper;
}

void LayoutTreeBuilderForText::CreateLayoutObject() {
  ComputedStyle& style = *style_;

  DCHECK(style_ == layout_object_parent_->Style() ||
         ToElement(LayoutTreeBuilderTraversal::Parent(*node_))
             ->HasDisplayContentsStyle());

  LayoutObject* next_layout_object;
  if (LayoutObject* wrapper = CreateInlineWrapperForDisplayContentsIfNeeded()) {
    layout_object_parent_ = wrapper;
    next_layout_object = nullptr;
  } else {
    next_layout_object = NextLayoutObject();
  }

  LayoutText* new_layout_object = node_->CreateTextLayoutObject(style);
  if (!layout_object_parent_->IsChildAllowed(new_layout_object, style)) {
    new_layout_object->Destroy();
    return;
  }

  // Make sure the LayoutObject already knows it is going to be added to a
  // LayoutFlowThread before we set the style for the first time. Otherwise code
  // using IsInsideFlowThread() in the StyleWillChange and StyleDidChange will
  // fail.
  new_layout_object->SetIsInsideFlowThread(
      layout_object_parent_->IsInsideFlowThread());

  node_->SetLayoutObject(new_layout_object);
  new_layout_object->SetStyle(&style);
  layout_object_parent_->AddChild(new_layout_object, next_layout_object);
}

}  // namespace blink
