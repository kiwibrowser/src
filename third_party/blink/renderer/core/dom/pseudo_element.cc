/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/dom/pseudo_element.h"

#include "third_party/blink/renderer/core/dom/element_rare_data.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_quote.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/content_data.h"

namespace blink {

PseudoElement* PseudoElement::Create(Element* parent, PseudoId pseudo_id) {
  return new PseudoElement(parent, pseudo_id);
}

const QualifiedName& PseudoElementTagName(PseudoId pseudo_id) {
  switch (pseudo_id) {
    case kPseudoIdAfter: {
      DEFINE_STATIC_LOCAL(QualifiedName, after,
                          (g_null_atom, "<pseudo:after>", g_null_atom));
      return after;
    }
    case kPseudoIdBefore: {
      DEFINE_STATIC_LOCAL(QualifiedName, before,
                          (g_null_atom, "<pseudo:before>", g_null_atom));
      return before;
    }
    case kPseudoIdBackdrop: {
      DEFINE_STATIC_LOCAL(QualifiedName, backdrop,
                          (g_null_atom, "<pseudo:backdrop>", g_null_atom));
      return backdrop;
    }
    case kPseudoIdFirstLetter: {
      DEFINE_STATIC_LOCAL(QualifiedName, first_letter,
                          (g_null_atom, "<pseudo:first-letter>", g_null_atom));
      return first_letter;
    }
    default:
      NOTREACHED();
  }
  DEFINE_STATIC_LOCAL(QualifiedName, name,
                      (g_null_atom, "<pseudo>", g_null_atom));
  return name;
}

String PseudoElement::PseudoElementNameForEvents(PseudoId pseudo_id) {
  DEFINE_STATIC_LOCAL(const String, after, ("::after"));
  DEFINE_STATIC_LOCAL(const String, before, ("::before"));
  switch (pseudo_id) {
    case kPseudoIdAfter:
      return after;
    case kPseudoIdBefore:
      return before;
    default:
      return g_empty_string;
  }
}

PseudoElement::PseudoElement(Element* parent, PseudoId pseudo_id)
    : Element(PseudoElementTagName(pseudo_id),
              &parent->GetDocument(),
              kCreatePseudoElement),
      pseudo_id_(pseudo_id) {
  DCHECK_NE(pseudo_id, kPseudoIdNone);
  parent->GetTreeScope().AdoptIfNeeded(*this);
  SetParentOrShadowHostNode(parent);
  SetHasCustomStyleCallbacks();
  if ((pseudo_id == kPseudoIdBefore || pseudo_id == kPseudoIdAfter) &&
      parent->HasTagName(HTMLNames::inputTag)) {
    UseCounter::Count(parent->GetDocument(),
                      WebFeature::kPseudoBeforeAfterForInputElement);
  }
}

scoped_refptr<ComputedStyle> PseudoElement::CustomStyleForLayoutObject() {
  scoped_refptr<ComputedStyle> original_style =
      ParentOrShadowHostElement()->PseudoStyle(PseudoStyleRequest(pseudo_id_));
  if (!original_style || original_style->Display() != EDisplay::kContents)
    return original_style;

  // For display:contents we should not generate a box, but we generate a non-
  // observable inline box for pseudo elements to be able to locate the
  // anonymous layout objects for generated content during DetachLayoutTree().
  scoped_refptr<ComputedStyle> layout_style = ComputedStyle::Create();
  layout_style->InheritFrom(*original_style);
  layout_style->SetContent(original_style->GetContentData());
  layout_style->SetDisplay(EDisplay::kInline);
  layout_style->SetStyleType(pseudo_id_);

  // Store the actual ComputedStyle to be able to return the correct values from
  // getComputedStyle().
  StoreNonLayoutObjectComputedStyle(original_style);
  return layout_style;
}

void PseudoElement::Dispose() {
  DCHECK(ParentOrShadowHostElement());

  probe::pseudoElementDestroyed(this);

  DCHECK(!nextSibling());
  DCHECK(!previousSibling());

  DetachLayoutTree();
  Element* parent = ParentOrShadowHostElement();
  GetDocument().AdoptIfNeeded(*this);
  SetParentOrShadowHostNode(nullptr);
  RemovedFrom(parent);
}

void PseudoElement::AttachLayoutTree(AttachContext& context) {
  DCHECK(!GetLayoutObject());

  Element::AttachLayoutTree(context);

  LayoutObject* layout_object = GetLayoutObject();
  if (!layout_object)
    return;

  ComputedStyle& style = layout_object->MutableStyleRef();
  if (style.StyleType() != kPseudoIdBefore &&
      style.StyleType() != kPseudoIdAfter)
    return;
  DCHECK(style.GetContentData());

  for (const ContentData* content = style.GetContentData(); content;
       content = content->Next()) {
    LayoutObject* child = content->CreateLayoutObject(*this, style);
    if (layout_object->IsChildAllowed(child, style)) {
      layout_object->AddChild(child);
      if (child->IsQuote())
        ToLayoutQuote(child)->AttachQuote();
    } else {
      child->Destroy();
    }
  }
}

bool PseudoElement::LayoutObjectIsNeeded(const ComputedStyle& style) const {
  return PseudoElementLayoutObjectIsNeeded(&style);
}

void PseudoElement::DidRecalcStyle(StyleRecalcChange) {
  if (!GetLayoutObject())
    return;

  // The layoutObjects inside pseudo elements are anonymous so they don't get
  // notified of recalcStyle and must have the style propagated downward
  // manually similar to LayoutObject::propagateStyleToAnonymousChildren.
  LayoutObject* layout_object = GetLayoutObject();
  for (LayoutObject* child = layout_object->NextInPreOrder(layout_object);
       child; child = child->NextInPreOrder(layout_object)) {
    // We only manage the style for the generated content items.
    if (!child->IsText() && !child->IsQuote() && !child->IsImage())
      continue;

    child->SetPseudoStyle(layout_object->MutableStyle());
  }
}

// With PseudoElements the DOM tree and Layout tree can differ. When you attach
// a, first-letter for example, into the DOM we walk down the Layout
// tree to find the correct insertion point for the LayoutObject. But, this
// means if we ask for the parentOrShadowHost Node from the first-letter
// pseudo element we will get some arbitrary ancestor of the LayoutObject.
//
// For hit testing, we need the parent Node of the LayoutObject for the
// first-letter pseudo element. So, by walking up the Layout tree we know
// we will get the parent and not some other ancestor.
Node* PseudoElement::FindAssociatedNode() const {
  // The ::backdrop element is parented to the LayoutView, not to the node
  // that it's associated with. We need to make sure ::backdrop sends the
  // events to the parent node correctly.
  if (GetPseudoId() == kPseudoIdBackdrop)
    return ParentOrShadowHostNode();

  DCHECK(GetLayoutObject());
  DCHECK(GetLayoutObject()->Parent());

  // We can have any number of anonymous layout objects inserted between
  // us and our parent so make sure we skip over them.
  LayoutObject* ancestor = GetLayoutObject()->Parent();
  while (ancestor->IsAnonymous() ||
         (ancestor->GetNode() && ancestor->GetNode()->IsPseudoElement())) {
    DCHECK(ancestor->Parent());
    ancestor = ancestor->Parent();
  }
  return ancestor->GetNode();
}

const ComputedStyle* PseudoElement::VirtualEnsureComputedStyle(
    PseudoId pseudo_element_specifier) {
  if (HasRareData()) {
    // Prefer NonLayoutObjectComputedStyle() for display:contents pseudos
    // instead of the ComputedStyle for the fictional inline box (see
    // CustomStyleForLayoutObject).
    if (const ComputedStyle* non_layout_computed_style =
            NonLayoutObjectComputedStyle()) {
      DCHECK(!GetLayoutObject() ||
             non_layout_computed_style->Display() == EDisplay::kContents);
      return non_layout_computed_style;
    }
  }
  return EnsureComputedStyle(pseudo_element_specifier);
}

bool PseudoElementLayoutObjectIsNeeded(const ComputedStyle* style) {
  if (!style)
    return false;
  if (style->Display() == EDisplay::kNone)
    return false;
  if (style->StyleType() == kPseudoIdFirstLetter ||
      style->StyleType() == kPseudoIdBackdrop)
    return true;
  return style->GetContentData();
}

}  // namespace blink
