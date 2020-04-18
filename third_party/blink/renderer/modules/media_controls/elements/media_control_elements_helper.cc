// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_elements_helper.h"

#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/html/html_div_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/layout/layout_slider.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_div_element.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_input_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

// static
bool MediaControlElementsHelper::IsUserInteractionEvent(Event* event) {
  const AtomicString& type = event->type();
  return type == EventTypeNames::pointerdown ||
         type == EventTypeNames::pointerup ||
         type == EventTypeNames::mousedown || type == EventTypeNames::mouseup ||
         type == EventTypeNames::click || type == EventTypeNames::dblclick ||
         event->IsKeyboardEvent() || event->IsTouchEvent();
}

// static
bool MediaControlElementsHelper::IsUserInteractionEventForSlider(
    Event* event,
    LayoutObject* layout_object) {
  // It is unclear if this can be converted to isUserInteractionEvent(), since
  // mouse* events seem to be eaten during a drag anyway, see
  // https://crbug.com/516416.
  if (IsUserInteractionEvent(event))
    return true;

  // Some events are only captured during a slider drag.
  const LayoutSlider* slider = ToLayoutSlider(layout_object);
  // TODO(crbug.com/695459#c1): LayoutSliderItem::inDragMode is incorrectly
  // false for drags that start from the track instead of the thumb.
  // Use SliderThumbElement::m_inDragMode and
  // SliderContainerElement::m_touchStarted instead.
  if (slider && !slider->InDragMode())
    return false;

  const AtomicString& type = event->type();
  return type == EventTypeNames::mouseover ||
         type == EventTypeNames::mouseout ||
         type == EventTypeNames::mousemove ||
         type == EventTypeNames::pointerover ||
         type == EventTypeNames::pointerout ||
         type == EventTypeNames::pointermove;
}

// static
MediaControlElementType MediaControlElementsHelper::GetMediaControlElementType(
    const Node* node) {
  SECURITY_DCHECK(node->IsMediaControlElement());
  const HTMLElement* element = ToHTMLElement(node);
  if (IsHTMLInputElement(*element))
    return static_cast<const MediaControlInputElement*>(element)->DisplayType();
  return static_cast<const MediaControlDivElement*>(element)->DisplayType();
}

// static
const HTMLMediaElement* MediaControlElementsHelper::ToParentMediaElement(
    const Node* node) {
  if (!node)
    return nullptr;
  const Node* shadow_host = node->OwnerShadowHost();
  if (!shadow_host)
    return nullptr;

  return IsHTMLMediaElement(shadow_host) ? ToHTMLMediaElement(shadow_host)
                                         : nullptr;
}

// static
HTMLDivElement* MediaControlElementsHelper::CreateDiv(const AtomicString& id,
                                                      ContainerNode* parent) {
  DCHECK(parent);
  HTMLDivElement* element = HTMLDivElement::Create(parent->GetDocument());
  element->SetShadowPseudoId(id);
  parent->ParserAppendChild(element);
  return element;
}

// static
WebSize MediaControlElementsHelper::GetSizeOrDefault(
    const Element& element,
    const WebSize& default_size) {
  float zoom_factor = 1.0f;
  int width = default_size.width;
  int height = default_size.height;

  if (LayoutBox* box = element.GetLayoutBox()) {
    width = box->LogicalWidth().Round();
    height = box->LogicalHeight().Round();
  }

  if (element.GetDocument().GetLayoutView())
    zoom_factor = element.GetDocument().GetLayoutView()->ZoomFactor();

  return WebSize(round(width / zoom_factor), round(height / zoom_factor));
}

// static
HTMLDivElement* MediaControlElementsHelper::CreateDivWithId(
    const AtomicString& id,
    ContainerNode* parent) {
  DCHECK(parent);
  HTMLDivElement* element = HTMLDivElement::Create(parent->GetDocument());
  element->setAttribute("id", id);
  parent->ParserAppendChild(element);
  return element;
}

// static
void MediaControlElementsHelper::NotifyMediaControlAccessibleFocus(
    Element* element) {
  const HTMLMediaElement* media_element = ToParentMediaElement(element);
  if (!media_element || !media_element->GetMediaControls())
    return;

  static_cast<MediaControlsImpl*>(media_element->GetMediaControls())
      ->OnAccessibleFocus();
}

}  // namespace blink
