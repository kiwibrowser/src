// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_BLINK_AX_ENUM_CONVERSION_H_
#define CONTENT_RENDERER_ACCESSIBILITY_BLINK_AX_ENUM_CONVERSION_H_

#include <stdint.h>

#include "third_party/blink/public/web/web_ax_object.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"

namespace content {

// Convert a Blink WebAXRole to an ax::mojom::Role defined in ui/accessibility.
ax::mojom::Role AXRoleFromBlink(blink::WebAXRole role);

// Convert a Blink WebAXEvent to an ax::mojom::Event defined in
// ui/accessibility.
ax::mojom::Event AXEventFromBlink(blink::WebAXEvent event);

// Provides a conversion between the WebAXObject state
// accessors and a state bitmask stored in an AXNodeData.
// (Note that some rare states are sent as boolean attributes
// in AXNodeData instead.)
void AXStateFromBlink(const blink::WebAXObject& o, ui::AXNodeData* dst);

ax::mojom::DefaultActionVerb AXDefaultActionVerbFromBlink(
    blink::WebAXDefaultActionVerb action_verb);

ax::mojom::MarkerType AXMarkerTypeFromBlink(blink::WebAXMarkerType marker_type);

ax::mojom::TextDirection AXTextDirectionFromBlink(
    blink::WebAXTextDirection text_direction);

ax::mojom::TextPosition AXTextPositionFromBlink(
    blink::WebAXTextPosition text_position);

ax::mojom::TextStyle AXTextStyleFromBlink(blink::WebAXTextStyle text_style);

ax::mojom::AriaCurrentState AXAriaCurrentStateFromBlink(
    blink::WebAXAriaCurrentState aria_current_state);

ax::mojom::HasPopup AXHasPopupFromBlink(blink::WebAXHasPopup has_popup);

ax::mojom::InvalidState AXInvalidStateFromBlink(
    blink::WebAXInvalidState invalid_state);

ax::mojom::CheckedState AXCheckedStateFromBlink(
    blink::WebAXCheckedState checked_state);

ax::mojom::SortDirection AXSortDirectionFromBlink(
    blink::WebAXSortDirection sort_direction);

ax::mojom::NameFrom AXNameFromFromBlink(blink::WebAXNameFrom name_from);

ax::mojom::DescriptionFrom AXDescriptionFromFromBlink(
    blink::WebAXDescriptionFrom description_from);

ax::mojom::TextAffinity AXTextAffinityFromBlink(
    blink::WebAXTextAffinity affinity);

}  // namespace content

#endif  // CONTENT_RENDERER_ACCESSIBILITY_BLINK_AX_ENUM_CONVERSION_H_
