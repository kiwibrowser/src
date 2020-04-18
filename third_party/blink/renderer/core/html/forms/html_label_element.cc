/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/forms/html_label_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_controller.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/forms/html_form_control_element.h"
#include "third_party/blink/renderer/core/html/forms/listed_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"

namespace blink {

using namespace HTMLNames;

inline HTMLLabelElement::HTMLLabelElement(Document& document)
    : HTMLElement(labelTag, document), processing_click_(false) {}

HTMLLabelElement* HTMLLabelElement::Create(Document& document) {
  return new HTMLLabelElement(document);
}

LabelableElement* HTMLLabelElement::control() const {
  const AtomicString& control_id = getAttribute(forAttr);
  if (control_id.IsNull()) {
    // Search the children and descendants of the label element for a form
    // element.
    // per http://dev.w3.org/html5/spec/Overview.html#the-label-element
    // the form element must be "labelable form-associated element".
    for (LabelableElement& element :
         Traversal<LabelableElement>::DescendantsOf(*this)) {
      if (element.SupportLabels()) {
        if (!element.IsFormControlElement()) {
          UseCounter::Count(
              GetDocument(),
              WebFeature::kHTMLLabelElementControlForNonFormAssociatedElement);
        }
        return &element;
      }
    }
    return nullptr;
  }

  if (!IsInTreeScope())
    return nullptr;

  if (Element* element = GetTreeScope().getElementById(control_id)) {
    if (IsLabelableElement(*element) &&
        ToLabelableElement(*element).SupportLabels()) {
      if (!element->IsFormControlElement()) {
        UseCounter::Count(
            GetDocument(),
            WebFeature::kHTMLLabelElementControlForNonFormAssociatedElement);
      }
      return ToLabelableElement(element);
    }
  }

  return nullptr;
}

HTMLFormElement* HTMLLabelElement::form() const {
  if (LabelableElement* control = this->control()) {
    return control->IsFormControlElement()
               ? ToHTMLFormControlElement(control)->Form()
               : nullptr;
  }
  return nullptr;
}

void HTMLLabelElement::SetActive(bool down) {
  if (down != IsActive()) {
    // Update our status first.
    HTMLElement::SetActive(down);
  }

  // Also update our corresponding control.
  HTMLElement* control_element = control();
  if (control_element && control_element->IsActive() != IsActive())
    control_element->SetActive(IsActive());
}

void HTMLLabelElement::SetHovered(bool over) {
  if (over != IsHovered()) {
    // Update our status first.
    HTMLElement::SetHovered(over);
  }

  // Also update our corresponding control.
  HTMLElement* element = control();
  if (element && element->IsHovered() != IsHovered())
    element->SetHovered(IsHovered());
}

bool HTMLLabelElement::IsInteractiveContent() const {
  return true;
}

bool HTMLLabelElement::IsInInteractiveContent(Node* node) const {
  if (!IsShadowIncludingInclusiveAncestorOf(node))
    return false;
  while (node && this != node) {
    if (node->IsHTMLElement() && ToHTMLElement(node)->IsInteractiveContent())
      return true;
    node = node->ParentOrShadowHostNode();
  }
  return false;
}

void HTMLLabelElement::DefaultEventHandler(Event* evt) {
  if (evt->type() == EventTypeNames::click && !processing_click_) {
    HTMLElement* element = control();

    // If we can't find a control or if the control received the click
    // event, then there's no need for us to do anything.
    if (!element ||
        (evt->target() && element->IsShadowIncludingInclusiveAncestorOf(
                              evt->target()->ToNode())))
      return;

    if (evt->target() && IsInInteractiveContent(evt->target()->ToNode()))
      return;

    //   Behaviour of label element is as follows:
    //     - If there is double click, two clicks will be passed to control
    //       element. Control element will *not* be focused.
    //     - If there is selection of label element by dragging, no click
    //       event is passed. Also, no focus on control element.
    //     - If there is already a selection on label element and then label
    //       is clicked, then click event is passed to control element and
    //       control element is focused.

    bool is_label_text_selected = false;

    // If the click is not simulated and the text of the label element
    // is selected by dragging over it, then return without passing the
    // click event to control element.
    // Note: check if it is a MouseEvent because a click event may
    // not be an instance of a MouseEvent if created by document.createEvent().
    if (evt->IsMouseEvent() && ToMouseEvent(evt)->HasPosition()) {
      if (LocalFrame* frame = GetDocument().GetFrame()) {
        // Check if there is a selection and click is not on the
        // selection.
        if (GetLayoutObject() && GetLayoutObject()->IsSelectable() &&
            frame->Selection()
                .ComputeVisibleSelectionInDOMTreeDeprecated()
                .IsRange() &&
            !frame->GetEventHandler()
                 .GetSelectionController()
                 .MouseDownWasSingleClickInSelection() &&
            evt->target()->ToNode()->CanStartSelection())
          is_label_text_selected = true;
        // If selection is there and is single click i.e. text is
        // selected by dragging over label text, then return.
        // Click count >=2, meaning double click or triple click,
        // should pass click event to control element.
        // Only in case of drag, *neither* we pass the click event,
        // *nor* we focus the control element.
        if (is_label_text_selected && ToMouseEvent(evt)->ClickCount() == 1)
          return;
      }
    }

    processing_click_ = true;

    GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();
    if (element->IsMouseFocusable()) {
      // If the label is *not* selected, or if the click happened on
      // selection of label, only then focus the control element.
      // In case of double click or triple click, selection will be there,
      // so do not focus the control element.
      if (!is_label_text_selected) {
        element->focus(FocusParams(SelectionBehaviorOnFocus::kRestore,
                                   kWebFocusTypeMouse, nullptr));
      }
    }

    // Click the corresponding control.
    element->DispatchSimulatedClick(evt);

    processing_click_ = false;

    evt->SetDefaultHandled();
  }

  HTMLElement::DefaultEventHandler(evt);
}

bool HTMLLabelElement::HasActivationBehavior() const {
  return true;
}

bool HTMLLabelElement::WillRespondToMouseClickEvents() {
  if (control() && control()->WillRespondToMouseClickEvents())
    return true;

  return HTMLElement::WillRespondToMouseClickEvents();
}

void HTMLLabelElement::focus(const FocusParams& params) {
  GetDocument().UpdateStyleAndLayoutTreeForNode(this);
  if (IsFocusable()) {
    HTMLElement::focus(params);
    return;
  }
  // To match other browsers, always restore previous selection.
  if (HTMLElement* element = control()) {
    element->focus(FocusParams(SelectionBehaviorOnFocus::kRestore, params.type,
                               params.source_capabilities, params.options));
  }
}

void HTMLLabelElement::AccessKeyAction(bool send_mouse_events) {
  if (HTMLElement* element = control())
    element->AccessKeyAction(send_mouse_events);
  else
    HTMLElement::AccessKeyAction(send_mouse_events);
}

}  // namespace blink
