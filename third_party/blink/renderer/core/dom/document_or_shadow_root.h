// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_OR_SHADOW_ROOT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_OR_SHADOW_ROOT_H_

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"

namespace blink {

class DocumentOrShadowRoot {
 public:
  static Element* activeElement(Document& document) {
    return document.ActiveElement();
  }

  static Element* activeElement(ShadowRoot& shadow_root) {
    return shadow_root.ActiveElement();
  }

  static StyleSheetList* styleSheets(Document& document) {
    return &document.StyleSheets();
  }

  static StyleSheetList* styleSheets(ShadowRoot& shadow_root) {
    return &shadow_root.StyleSheets();
  }

  static StyleSheetList* moreStyleSheets(TreeScope& tree_scope) {
    CHECK(RuntimeEnabledFeatures::ConstructableStylesheetsEnabled());
    return &tree_scope.MoreStyleSheets();
  }

  static void setMoreStyleSheets(TreeScope& tree_scope,
                                 StyleSheetList* more_style_sheets) {
    CHECK(RuntimeEnabledFeatures::ConstructableStylesheetsEnabled());
    tree_scope.SetMoreStyleSheets(more_style_sheets);
  }

  static DOMSelection* getSelection(TreeScope& tree_scope) {
    return tree_scope.GetSelection();
  }

  static Element* elementFromPoint(TreeScope& tree_scope, double x, double y) {
    return tree_scope.ElementFromPoint(x, y);
  }

  static HeapVector<Member<Element>> elementsFromPoint(TreeScope& tree_scope,
                                                       double x,
                                                       double y) {
    return tree_scope.ElementsFromPoint(x, y);
  }

  static Element* pointerLockElement(Document& document) {
    UseCounter::Count(document, WebFeature::kDocumentPointerLockElement);
    const Element* target = document.PointerLockElement();
    if (!target)
      return nullptr;
    // For Shadow DOM V0 compatibility: We allow returning an element in V0
    // shadow tree, even though it leaks the Shadow DOM.
    // TODO(kochi): Once V0 code is removed, the following V0 check is
    // unnecessary.
    if (target && target->IsInV0ShadowTree()) {
      UseCounter::Count(document,
                        WebFeature::kDocumentPointerLockElementInV0Shadow);
      return const_cast<Element*>(target);
    }
    return document.AdjustedElement(*target);
  }

  static Element* pointerLockElement(ShadowRoot& shadow_root) {
    // TODO(kochi): Once V0 code is removed, the following non-V1 check is
    // unnecessary.  After V0 code is removed, we can use the same logic for
    // Document and ShadowRoot.
    if (!shadow_root.IsV1())
      return nullptr;
    UseCounter::Count(shadow_root.GetDocument(),
                      WebFeature::kShadowRootPointerLockElement);
    const Element* target = shadow_root.GetDocument().PointerLockElement();
    if (!target)
      return nullptr;
    return shadow_root.AdjustedElement(*target);
  }

  static Element* fullscreenElement(TreeScope& scope) {
    return Fullscreen::FullscreenElementForBindingFrom(scope);
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_OR_SHADOW_ROOT_H_
