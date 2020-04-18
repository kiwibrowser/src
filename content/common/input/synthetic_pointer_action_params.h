// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_SYNTHETIC_POINTER_ACTION_PARAMS_H_
#define CONTENT_COMMON_INPUT_SYNTHETIC_POINTER_ACTION_PARAMS_H_

#include "base/logging.h"
#include "content/common/content_export.h"
#include "content/common/input/input_param_traits.h"
#include "content/common/input/synthetic_gesture_params.h"
#include "content/common/input/synthetic_web_input_event_builders.h"
#include "third_party/blink/public/platform/web_touch_event.h"
#include "ui/gfx/geometry/point_f.h"

namespace ipc_fuzzer {
template <class T>
struct FuzzTraits;
}  // namespace ipc_fuzzer

namespace content {

struct CONTENT_EXPORT SyntheticPointerActionParams {
 public:
  // All the pointer actions that will be dispatched together will be grouped
  // in an array.
  enum class PointerActionType {
    NOT_INITIALIZED,
    PRESS,
    MOVE,
    RELEASE,
    IDLE,
    POINTER_ACTION_TYPE_MAX = IDLE
  };

  enum class Button {
    LEFT,
    MIDDLE,
    RIGHT,
    BACK,
    FORWARD,
    BUTTON_MAX = FORWARD
  };

  SyntheticPointerActionParams();
  SyntheticPointerActionParams(PointerActionType action_type);
  ~SyntheticPointerActionParams();

  void set_pointer_action_type(PointerActionType pointer_action_type) {
    pointer_action_type_ = pointer_action_type;
  }

  void set_index(int index) {
    DCHECK_GE(index, 0);
    DCHECK_LT(index, blink::WebTouchEvent::kTouchesLengthCap);
    index_ = index;
  }

  void set_position(const gfx::PointF& position) {
    DCHECK(pointer_action_type_ == PointerActionType::PRESS ||
           pointer_action_type_ == PointerActionType::MOVE);
    position_ = position;
  }

  void set_button(Button button) {
    DCHECK(pointer_action_type_ == PointerActionType::PRESS ||
           pointer_action_type_ == PointerActionType::RELEASE);
    button_ = button;
  }

  PointerActionType pointer_action_type() const { return pointer_action_type_; }

  int index() const {
    DCHECK_GE(index_, 0);
    DCHECK_LT(index_, blink::WebTouchEvent::kTouchesLengthCap);
    return index_;
  }

  gfx::PointF position() const {
    DCHECK(pointer_action_type_ == PointerActionType::PRESS ||
           pointer_action_type_ == PointerActionType::MOVE);
    return position_;
  }

  Button button() const {
    DCHECK(pointer_action_type_ == PointerActionType::PRESS ||
           pointer_action_type_ == PointerActionType::RELEASE);
    return button_;
  }

  static unsigned GetWebMouseEventModifier(
      SyntheticPointerActionParams::Button button);
  static blink::WebMouseEvent::Button GetWebMouseEventButton(
      SyntheticPointerActionParams::Button button);

 private:
  friend struct IPC::ParamTraits<content::SyntheticPointerActionParams>;
  friend struct ipc_fuzzer::FuzzTraits<content::SyntheticPointerActionParams>;

  PointerActionType pointer_action_type_;
  // The position of the pointer, where it presses or moves to.
  gfx::PointF position_;
  // The index of the pointer in the pointer action sequence passed from the
  // user API.
  int index_;
  Button button_;
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_SYNTHETIC_POINTER_ACTION_PARAMS_H_
