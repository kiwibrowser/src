// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/selection_sample.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

namespace {

Element* GetOrCreateElement(ContainerNode* parent,
                            const HTMLQualifiedName& tag_name) {
  HTMLCollection* elements = parent->getElementsByTagNameNS(
      tag_name.NamespaceURI(), tag_name.LocalName());
  if (!elements->IsEmpty())
    return elements->item(0);
  return parent->ownerDocument()->CreateRawElement(
      tag_name, CreateElementFlags::ByCreateElement());
}

}  // namespace

EditingTestBase::EditingTestBase() = default;

EditingTestBase::~EditingTestBase() = default;

void EditingTestBase::InsertStyleElement(const std::string& style_rules) {
  Element* const head = GetOrCreateElement(&GetDocument(), HTMLNames::headTag);
  DCHECK_EQ(head, GetOrCreateElement(&GetDocument(), HTMLNames::headTag));
  Element* const style = GetDocument().CreateRawElement(
      HTMLNames::styleTag, CreateElementFlags::ByCreateElement());
  style->setTextContent(String(style_rules.data(), style_rules.size()));
  head->appendChild(style);
}

Position EditingTestBase::SetCaretTextToBody(
    const std::string& selection_text) {
  const SelectionInDOMTree selection = SetSelectionTextToBody(selection_text);
  DCHECK(selection.IsCaret())
      << "|selection_text| should contain a caret marker '|'";
  return selection.Base();
}

SelectionInDOMTree EditingTestBase::SetSelectionTextToBody(
    const std::string& selection_text) {
  return SetSelectionText(GetDocument().body(), selection_text);
}

SelectionInDOMTree EditingTestBase::SetSelectionText(
    HTMLElement* element,
    const std::string& selection_text) {
  const SelectionInDOMTree selection =
      SelectionSample::SetSelectionText(element, selection_text);
  UpdateAllLifecyclePhases();
  return selection;
}

std::string EditingTestBase::GetSelectionTextFromBody(
    const SelectionInDOMTree& selection) const {
  return SelectionSample::GetSelectionText(*GetDocument().body(), selection);
}

std::string EditingTestBase::GetSelectionTextFromBody() const {
  return GetSelectionTextFromBody(Selection().GetSelectionInDOMTree());
}

std::string EditingTestBase::GetSelectionTextInFlatTreeFromBody(
    const SelectionInFlatTree& selection) const {
  return SelectionSample::GetSelectionTextInFlatTree(*GetDocument().body(),
                                                     selection);
}

std::string EditingTestBase::GetCaretTextFromBody(
    const Position& position) const {
  DCHECK(position.IsValidFor(GetDocument()))
      << "A valid position must be provided " << position;
  return GetSelectionTextFromBody(
      SelectionInDOMTree::Builder().Collapse(position).Build());
}

ShadowRoot* EditingTestBase::CreateShadowRootForElementWithIDAndSetInnerHTML(
    TreeScope& scope,
    const char* host_element_id,
    const char* shadow_root_content) {
  ShadowRoot& shadow_root =
      scope.getElementById(AtomicString::FromUTF8(host_element_id))
          ->CreateV0ShadowRootForTesting();
  shadow_root.SetInnerHTMLFromString(String::FromUTF8(shadow_root_content),
                                     ASSERT_NO_EXCEPTION);
  scope.GetDocument().View()->UpdateAllLifecyclePhases();
  return &shadow_root;
}

ShadowRoot* EditingTestBase::SetShadowContent(const char* shadow_content,
                                              const char* host) {
  ShadowRoot* shadow_root = CreateShadowRootForElementWithIDAndSetInnerHTML(
      GetDocument(), host, shadow_content);
  return shadow_root;
}

}  // namespace blink
