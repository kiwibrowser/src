/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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

#include "third_party/blink/renderer/core/html/forms/listed_element.h"

#include "third_party/blink/renderer/core/dom/id_target_observer.h"
#include "third_party/blink/renderer/core/dom/node_traversal.h"
#include "third_party/blink/renderer/core/html/forms/html_form_control_element.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/forms/validity_state.h"
#include "third_party/blink/renderer/core/html/html_object_element.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

using namespace HTMLNames;

class FormAttributeTargetObserver : public IdTargetObserver {
 public:
  static FormAttributeTargetObserver* Create(const AtomicString& id,
                                             ListedElement*);
  void Trace(blink::Visitor*) override;
  void IdTargetChanged() override;

 private:
  FormAttributeTargetObserver(const AtomicString& id, ListedElement*);

  Member<ListedElement> element_;
};

ListedElement::ListedElement() : form_was_set_by_parser_(false) {}

ListedElement::~ListedElement() {
  // We can't call setForm here because it contains virtual calls.
}

void ListedElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(form_attribute_target_observer_);
  visitor->Trace(form_);
  visitor->Trace(validity_state_);
}

ValidityState* ListedElement::validity() {
  if (!validity_state_)
    validity_state_ = ValidityState::Create(this);

  return validity_state_.Get();
}

void ListedElement::DidMoveToNewDocument(Document& old_document) {
  HTMLElement* element = ToHTMLElement(this);
  if (element->FastHasAttribute(formAttr))
    SetFormAttributeTargetObserver(nullptr);
}

void ListedElement::InsertedInto(ContainerNode* insertion_point) {
  if (!form_was_set_by_parser_ || !form_ ||
      NodeTraversal::HighestAncestorOrSelf(*insertion_point) !=
          NodeTraversal::HighestAncestorOrSelf(*form_.Get()))
    ResetFormOwner();

  if (!insertion_point->isConnected())
    return;

  HTMLElement* element = ToHTMLElement(this);
  if (element->FastHasAttribute(formAttr))
    ResetFormAttributeTargetObserver();
}

void ListedElement::RemovedFrom(ContainerNode* insertion_point) {
  HTMLElement* element = ToHTMLElement(this);
  if (insertion_point->isConnected() && element->FastHasAttribute(formAttr)) {
    SetFormAttributeTargetObserver(nullptr);
    ResetFormOwner();
    return;
  }
  // If the form and element are both in the same tree, preserve the connection
  // to the form.  Otherwise, null out our form and remove ourselves from the
  // form's list of elements.
  if (form_ && NodeTraversal::HighestAncestorOrSelf(*element) !=
                   NodeTraversal::HighestAncestorOrSelf(*form_.Get()))
    ResetFormOwner();
}

HTMLFormElement* ListedElement::FindAssociatedForm(
    const HTMLElement* element,
    const AtomicString& form_id,
    HTMLFormElement* form_ancestor) {
  // 3. If the element is reassociateable, has a form content attribute, and
  // is itself in a Document, then run these substeps:
  if (!form_id.IsNull() && element->isConnected()) {
    // 3.1. If the first element in the Document to have an ID that is
    // case-sensitively equal to the element's form content attribute's
    // value is a form element, then associate the form-associated element
    // with that form element.
    // 3.2. Abort the "reset the form owner" steps.
    Element* new_form_candidate =
        element->GetTreeScope().getElementById(form_id);
    return ToHTMLFormElementOrNull(new_form_candidate);
  }
  // 4. Otherwise, if the form-associated element in question has an ancestor
  // form element, then associate the form-associated element with the nearest
  // such ancestor form element.
  return form_ancestor;
}

void ListedElement::FormRemovedFromTree(const Node& form_root) {
  DCHECK(form_);
  if (NodeTraversal::HighestAncestorOrSelf(ToHTMLElement(*this)) == form_root)
    return;
  ResetFormOwner();
}

void ListedElement::AssociateByParser(HTMLFormElement* form) {
  if (form && form->isConnected()) {
    form_was_set_by_parser_ = true;
    SetForm(form);
    form->DidAssociateByParser();
  }
}

void ListedElement::SetForm(HTMLFormElement* new_form) {
  if (form_.Get() == new_form)
    return;
  WillChangeForm();
  if (form_)
    form_->Disassociate(*this);
  if (new_form) {
    form_ = new_form;
    form_->Associate(*this);
  } else {
    form_ = nullptr;
  }
  DidChangeForm();
}

void ListedElement::WillChangeForm() {}

void ListedElement::DidChangeForm() {
  if (!form_was_set_by_parser_ && form_ && form_->isConnected()) {
    HTMLElement* element = ToHTMLElement(this);
    element->GetDocument().DidAssociateFormControl(element);
  }
}

void ListedElement::ResetFormOwner() {
  form_was_set_by_parser_ = false;
  HTMLElement* element = ToHTMLElement(this);
  const AtomicString& form_id(element->FastGetAttribute(formAttr));
  HTMLFormElement* nearest_form = element->FindFormAncestor();
  // 1. If the element's form owner is not null, and either the element is not
  // reassociateable or its form content attribute is not present, and the
  // element's form owner is its nearest form element ancestor after the
  // change to the ancestor chain, then do nothing, and abort these steps.
  if (form_ && form_id.IsNull() && form_.Get() == nearest_form)
    return;

  SetForm(FindAssociatedForm(element, form_id, nearest_form));
}

void ListedElement::FormAttributeChanged() {
  ResetFormOwner();
  ResetFormAttributeTargetObserver();
}

bool ListedElement::CustomError() const {
  const HTMLElement* element = ToHTMLElement(this);
  return element->willValidate() && !custom_validation_message_.IsEmpty();
}

bool ListedElement::HasBadInput() const {
  return false;
}

bool ListedElement::PatternMismatch() const {
  return false;
}

bool ListedElement::RangeOverflow() const {
  return false;
}

bool ListedElement::RangeUnderflow() const {
  return false;
}

bool ListedElement::StepMismatch() const {
  return false;
}

bool ListedElement::TooLong() const {
  return false;
}

bool ListedElement::TooShort() const {
  return false;
}

bool ListedElement::TypeMismatch() const {
  return false;
}

bool ListedElement::Valid() const {
  bool some_error = TypeMismatch() || StepMismatch() || RangeUnderflow() ||
                    RangeOverflow() || TooLong() || TooShort() ||
                    PatternMismatch() || ValueMissing() || HasBadInput() ||
                    CustomError();
  return !some_error;
}

bool ListedElement::ValueMissing() const {
  return false;
}

String ListedElement::CustomValidationMessage() const {
  return custom_validation_message_;
}

String ListedElement::validationMessage() const {
  return CustomError() ? custom_validation_message_ : String();
}

String ListedElement::ValidationSubMessage() const {
  return String();
}

void ListedElement::setCustomValidity(const String& error) {
  custom_validation_message_ = error;
}

void ListedElement::SetFormAttributeTargetObserver(
    FormAttributeTargetObserver* new_observer) {
  if (form_attribute_target_observer_)
    form_attribute_target_observer_->Unregister();
  form_attribute_target_observer_ = new_observer;
}

void ListedElement::ResetFormAttributeTargetObserver() {
  HTMLElement* element = ToHTMLElement(this);
  const AtomicString& form_id(element->FastGetAttribute(formAttr));
  if (!form_id.IsNull() && element->isConnected()) {
    SetFormAttributeTargetObserver(
        FormAttributeTargetObserver::Create(form_id, this));
  } else {
    SetFormAttributeTargetObserver(nullptr);
  }
}

void ListedElement::FormAttributeTargetChanged() {
  ResetFormOwner();
}

const AtomicString& ListedElement::GetName() const {
  const AtomicString& name = ToHTMLElement(this)->GetNameAttribute();
  return name.IsNull() ? g_empty_atom : name;
}

bool ListedElement::IsFormControlElementWithState() const {
  return false;
}

const HTMLElement& ToHTMLElement(const ListedElement& listed_element) {
  if (listed_element.IsFormControlElement())
    return ToHTMLFormControlElement(listed_element);
  return ToHTMLObjectElementFromListedElement(listed_element);
}

const HTMLElement* ToHTMLElement(const ListedElement* listed_element) {
  DCHECK(listed_element);
  return &ToHTMLElement(*listed_element);
}

HTMLElement* ToHTMLElement(ListedElement* listed_element) {
  return const_cast<HTMLElement*>(
      ToHTMLElement(static_cast<const ListedElement*>(listed_element)));
}

HTMLElement& ToHTMLElement(ListedElement& listed_element) {
  return const_cast<HTMLElement&>(
      ToHTMLElement(static_cast<const ListedElement&>(listed_element)));
}

FormAttributeTargetObserver* FormAttributeTargetObserver::Create(
    const AtomicString& id,
    ListedElement* element) {
  return new FormAttributeTargetObserver(id, element);
}

FormAttributeTargetObserver::FormAttributeTargetObserver(const AtomicString& id,
                                                         ListedElement* element)
    : IdTargetObserver(
          ToHTMLElement(element)->GetTreeScope().GetIdTargetObserverRegistry(),
          id),
      element_(element) {}

void FormAttributeTargetObserver::Trace(blink::Visitor* visitor) {
  visitor->Trace(element_);
  IdTargetObserver::Trace(visitor);
}

void FormAttributeTargetObserver::IdTargetChanged() {
  element_->FormAttributeTargetChanged();
}

}  // namespace blink
