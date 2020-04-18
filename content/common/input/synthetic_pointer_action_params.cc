// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/synthetic_pointer_action_params.h"

namespace content {

SyntheticPointerActionParams::SyntheticPointerActionParams()
    : pointer_action_type_(PointerActionType::NOT_INITIALIZED),
      index_(0),
      button_(Button::LEFT) {}

SyntheticPointerActionParams::SyntheticPointerActionParams(
    PointerActionType action_type)
    : pointer_action_type_(action_type), index_(0), button_(Button::LEFT) {}

SyntheticPointerActionParams::~SyntheticPointerActionParams() {}

// static
unsigned SyntheticPointerActionParams::GetWebMouseEventModifier(
    SyntheticPointerActionParams::Button button) {
  switch (button) {
    case SyntheticPointerActionParams::Button::LEFT:
      return blink::WebMouseEvent::kLeftButtonDown;
    case SyntheticPointerActionParams::Button::MIDDLE:
      return blink::WebMouseEvent::kMiddleButtonDown;
    case SyntheticPointerActionParams::Button::RIGHT:
      return blink::WebMouseEvent::kRightButtonDown;
    case SyntheticPointerActionParams::Button::BACK:
      return blink::WebMouseEvent::kBackButtonDown;
    case SyntheticPointerActionParams::Button::FORWARD:
      return blink::WebMouseEvent::kForwardButtonDown;
  }
  NOTREACHED();
  return blink::WebMouseEvent::kNoModifiers;
}

// static
blink::WebMouseEvent::Button
SyntheticPointerActionParams::GetWebMouseEventButton(
    SyntheticPointerActionParams::Button button) {
  switch (button) {
    case SyntheticPointerActionParams::Button::LEFT:
      return blink::WebMouseEvent::Button::kLeft;
    case SyntheticPointerActionParams::Button::MIDDLE:
      return blink::WebMouseEvent::Button::kMiddle;
    case SyntheticPointerActionParams::Button::RIGHT:
      return blink::WebMouseEvent::Button::kRight;
    case SyntheticPointerActionParams::Button::BACK:
      return blink::WebMouseEvent::Button::kBack;
    case SyntheticPointerActionParams::Button::FORWARD:
      return blink::WebMouseEvent::Button::kForward;
  }
  NOTREACHED();
  return blink::WebMouseEvent::Button::kNoButton;
}

}  // namespace content
