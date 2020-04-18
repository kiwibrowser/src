/**
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved.
 *               (http://www.torchmobile.com/)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "third_party/blink/renderer/core/layout/layout_search_field.h"

#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/shadow/shadow_element_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"

namespace blink {

using namespace HTMLNames;

// ----------------------------

LayoutSearchField::LayoutSearchField(HTMLInputElement* element)
    : LayoutTextControlSingleLine(element) {
  DCHECK_EQ(element->type(), InputTypeNames::search);
}

LayoutSearchField::~LayoutSearchField() = default;

inline Element* LayoutSearchField::SearchDecorationElement() const {
  return InputElement()->UserAgentShadowRoot()->getElementById(
      ShadowElementNames::SearchDecoration());
}

inline Element* LayoutSearchField::CancelButtonElement() const {
  return InputElement()->UserAgentShadowRoot()->getElementById(
      ShadowElementNames::ClearButton());
}

LayoutUnit LayoutSearchField::ComputeControlLogicalHeight(
    LayoutUnit line_height,
    LayoutUnit non_content_height) const {
  Element* search_decoration = SearchDecorationElement();
  if (LayoutBox* decoration_layout_object =
          search_decoration ? search_decoration->GetLayoutBox() : nullptr) {
    decoration_layout_object->UpdateLogicalHeight();
    non_content_height =
        max(non_content_height,
            decoration_layout_object->BorderAndPaddingLogicalHeight() +
                decoration_layout_object->MarginLogicalHeight());
    line_height = max(line_height, decoration_layout_object->LogicalHeight());
  }
  Element* cancel_button = CancelButtonElement();
  if (LayoutBox* cancel_layout_object =
          cancel_button ? cancel_button->GetLayoutBox() : nullptr) {
    cancel_layout_object->UpdateLogicalHeight();
    non_content_height =
        max(non_content_height,
            cancel_layout_object->BorderAndPaddingLogicalHeight() +
                cancel_layout_object->MarginLogicalHeight());
    line_height = max(line_height, cancel_layout_object->LogicalHeight());
  }

  return line_height + non_content_height;
}

}  // namespace blink
