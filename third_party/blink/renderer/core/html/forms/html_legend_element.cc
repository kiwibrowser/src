/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/forms/html_legend_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/html/forms/html_field_set_element.h"
#include "third_party/blink/renderer/core/html/forms/html_form_control_element.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

using namespace HTMLNames;

inline HTMLLegendElement::HTMLLegendElement(Document& document)
    : HTMLElement(legendTag, document) {}

DEFINE_NODE_FACTORY(HTMLLegendElement)

HTMLFormControlElement* HTMLLegendElement::AssociatedControl() {
  // Check if there's a fieldset belonging to this legend.
  HTMLFieldSetElement* fieldset =
      Traversal<HTMLFieldSetElement>::FirstAncestor(*this);
  if (!fieldset)
    return nullptr;

  // Find first form element inside the fieldset that is not a legend element.
  // FIXME: Should we consider tabindex?
  return Traversal<HTMLFormControlElement>::Next(*fieldset, fieldset);
}

void HTMLLegendElement::focus(const FocusParams& params) {
  GetDocument().UpdateStyleAndLayoutTreeForNode(this);
  if (IsFocusable()) {
    Element::focus(params);
    return;
  }

  // To match other browsers' behavior, never restore previous selection.
  if (HTMLFormControlElement* control = AssociatedControl()) {
    control->focus(FocusParams(SelectionBehaviorOnFocus::kReset, params.type,
                               params.source_capabilities, params.options));
  }
}

void HTMLLegendElement::AccessKeyAction(bool send_mouse_events) {
  if (HTMLFormControlElement* control = AssociatedControl())
    control->AccessKeyAction(send_mouse_events);
}

HTMLFormElement* HTMLLegendElement::form() const {
  // According to the specification, If the legend has a fieldset element as
  // its parent, then the form attribute must return the same value as the
  // form attribute on that fieldset element. Otherwise, it must return null.
  if (auto* fieldset = ToHTMLFieldSetElementOrNull(parentNode()))
    return fieldset->formOwner();
  return nullptr;
}

}  // namespace blink
