// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_input_handler.h"

#include <stdint.h>

#include "base/logging.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/mouse_cursor.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/touch_point.h"
#include "ppapi/cpp/var.h"
#include "remoting/proto/event.pb.h"
#include "remoting/protocol/input_event_tracker.h"
#include "remoting/protocol/input_stub.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace remoting {

namespace {

void SetTouchEventType(PP_InputEvent_Type pp_type,
                       protocol::TouchEvent* touch_event) {
  DCHECK(touch_event);
  switch (pp_type) {
    case PP_INPUTEVENT_TYPE_TOUCHSTART:
      touch_event->set_event_type(protocol::TouchEvent::TOUCH_POINT_START);
      return;
    case PP_INPUTEVENT_TYPE_TOUCHMOVE:
      touch_event->set_event_type(protocol::TouchEvent::TOUCH_POINT_MOVE);
      return;
    case PP_INPUTEVENT_TYPE_TOUCHEND:
      touch_event->set_event_type(protocol::TouchEvent::TOUCH_POINT_END);
      return;
    case PP_INPUTEVENT_TYPE_TOUCHCANCEL:
      touch_event->set_event_type(protocol::TouchEvent::TOUCH_POINT_CANCEL);
      return;
    default:
      NOTREACHED() << "Unknown event type: " << pp_type;
      return;
  }
}

// Creates a protocol::TouchEvent instance from |pp_touch_event|.
// Note that only the changed touches are added to the TouchEvent.
protocol::TouchEvent MakeTouchEvent(const pp::TouchInputEvent& pp_touch_event) {
  protocol::TouchEvent touch_event;
  SetTouchEventType(pp_touch_event.GetType(), &touch_event);
  DCHECK(touch_event.has_event_type());

  for (uint32_t i = 0;
       i < pp_touch_event.GetTouchCount(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES);
       ++i) {
    pp::TouchPoint pp_point =
        pp_touch_event.GetTouchByIndex(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES, i);
    protocol::TouchEventPoint* point = touch_event.add_touch_points();
    point->set_id(pp_point.id());
    point->set_x(pp_point.position().x());
    point->set_y(pp_point.position().y());
    point->set_radius_x(pp_point.radii().x());
    point->set_radius_y(pp_point.radii().y());
    point->set_angle(pp_point.rotation_angle());
  }

  return touch_event;
}

// Builds the Chromotocol lock states flags for the PPAPI |event|.
void SetLockStates(protocol::KeyEvent* key_event,
                   const pp::InputEvent& pp_key_event) {
  uint32_t modifiers = pp_key_event.GetModifiers();
  bool caps_lock = (modifiers & PP_INPUTEVENT_MODIFIER_CAPSLOCKKEY) != 0;
  bool num_lock = (modifiers & PP_INPUTEVENT_MODIFIER_NUMLOCKKEY) != 0;

  // Set the new discrete lock state fields
  key_event->set_caps_lock_state(caps_lock);
  key_event->set_num_lock_state(num_lock);

  // Also set the legacy lock_states field to support older hosts.
  uint32_t lock_states = 0;
  if (caps_lock) {
    lock_states |= protocol::KeyEvent::LOCK_STATES_CAPSLOCK;
  }
  if (num_lock) {
    lock_states |= protocol::KeyEvent::LOCK_STATES_NUMLOCK;
  }
  key_event->set_lock_states(lock_states);
}

// Builds a protocol::KeyEvent from the supplied PPAPI event.
protocol::KeyEvent MakeKeyEvent(const pp::KeyboardInputEvent& pp_key_event) {
  protocol::KeyEvent key_event;
  std::string dom_code = pp_key_event.GetCode().AsString();
  // Chrome M52 changed the string representation of the left and right OS
  // keys, which means that if the client plugin is compiled against a
  // different version of the mapping table, the lookup will fail. The long-
  // term solution is to use JavaScript input events, but for now just check
  // explicitly for the old names and convert them to the new ones.
  if (dom_code == "OSLeft") {
    dom_code = "MetaLeft";
  } else if (dom_code == "OSRight") {
    dom_code = "MetaRight";
  }
  key_event.set_usb_keycode(
      ui::KeycodeConverter::CodeStringToUsbKeycode(dom_code));
  key_event.set_pressed(pp_key_event.GetType() == PP_INPUTEVENT_TYPE_KEYDOWN);
  SetLockStates(&key_event, pp_key_event);
  return key_event;
}

// Builds a protocol::MouseEvent from the supplied PPAPI event.
protocol::MouseEvent MakeMouseEvent(const pp::MouseInputEvent& pp_mouse_event,
                                    bool set_deltas) {
  protocol::MouseEvent mouse_event;
  mouse_event.set_x(pp_mouse_event.GetPosition().x());
  mouse_event.set_y(pp_mouse_event.GetPosition().y());
  if (set_deltas) {
    pp::Point delta = pp_mouse_event.GetMovement();
    mouse_event.set_delta_x(delta.x());
    mouse_event.set_delta_y(delta.y());
  }
  return mouse_event;
}

}  // namespace

PepperInputHandler::PepperInputHandler(
    protocol::InputEventTracker* input_tracker)
    : input_tracker_(input_tracker),
      has_focus_(false),
      send_mouse_input_when_unfocused_(false),
      send_mouse_move_deltas_(false),
      wheel_delta_x_(0),
      wheel_delta_y_(0),
      wheel_ticks_x_(0),
      wheel_ticks_y_(0),
      detect_stuck_modifiers_(false) {
}

bool PepperInputHandler::HandleInputEvent(const pp::InputEvent& event) {
  if (detect_stuck_modifiers_)
    ReleaseAllIfModifiersStuck(event);

  switch (event.GetType()) {
    // Touch input cases.
    case PP_INPUTEVENT_TYPE_TOUCHSTART:
    case PP_INPUTEVENT_TYPE_TOUCHMOVE:
    case PP_INPUTEVENT_TYPE_TOUCHEND:
    case PP_INPUTEVENT_TYPE_TOUCHCANCEL: {
      pp::TouchInputEvent pp_touch_event(event);
      input_tracker_->InjectTouchEvent(MakeTouchEvent(pp_touch_event));
      return true;
    }

    case PP_INPUTEVENT_TYPE_CONTEXTMENU: {
      // We need to return true here or else we'll get a local (plugin) context
      // menu instead of the mouseup event for the right click.
      return true;
    }

    case PP_INPUTEVENT_TYPE_KEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYUP: {
      pp::KeyboardInputEvent pp_key_event(event);
      input_tracker_->InjectKeyEvent(MakeKeyEvent(pp_key_event));
      return true;
    }

    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_MOUSEUP: {
      if (!has_focus_ && !send_mouse_input_when_unfocused_)
        return false;

      pp::MouseInputEvent pp_mouse_event(event);
      protocol::MouseEvent mouse_event(
          MakeMouseEvent(pp_mouse_event, send_mouse_move_deltas_));
      switch (pp_mouse_event.GetButton()) {
        case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
          mouse_event.set_button(protocol::MouseEvent::BUTTON_LEFT);
          break;
        case PP_INPUTEVENT_MOUSEBUTTON_MIDDLE:
          mouse_event.set_button(protocol::MouseEvent::BUTTON_MIDDLE);
          break;
        case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
          mouse_event.set_button(protocol::MouseEvent::BUTTON_RIGHT);
          break;
        case PP_INPUTEVENT_MOUSEBUTTON_NONE:
          break;
      }
      if (mouse_event.has_button()) {
        bool is_down = (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEDOWN);
        mouse_event.set_button_down(is_down);
        input_tracker_->InjectMouseEvent(mouse_event);
      }

      return true;
    }

    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
    case PP_INPUTEVENT_TYPE_MOUSELEAVE: {
      if (!has_focus_ && !send_mouse_input_when_unfocused_)
        return false;

      pp::MouseInputEvent pp_mouse_event(event);
      input_tracker_->InjectMouseEvent(
          MakeMouseEvent(pp_mouse_event, send_mouse_move_deltas_));

      return true;
    }

    case PP_INPUTEVENT_TYPE_WHEEL: {
      if (!has_focus_ && !send_mouse_input_when_unfocused_)
        return false;

      pp::WheelInputEvent pp_wheel_event(event);

      // Ignore scroll-by-page events, for now.
      if (pp_wheel_event.GetScrollByPage())
        return true;

      // Add this event to our accumulated sub-pixel deltas and clicks.
      pp::FloatPoint delta = pp_wheel_event.GetDelta();
      wheel_delta_x_ += delta.x();
      wheel_delta_y_ += delta.y();
      pp::FloatPoint ticks = pp_wheel_event.GetTicks();
      wheel_ticks_x_ += ticks.x();
      wheel_ticks_y_ += ticks.y();

      // If there is at least a pixel's movement, emit an event. We don't
      // ever expect to accumulate one tick's worth of scrolling without
      // accumulating a pixel's worth at the same time, so this is safe.
      int delta_x = static_cast<int>(wheel_delta_x_);
      int delta_y = static_cast<int>(wheel_delta_y_);
      if (delta_x != 0 || delta_y != 0) {
        wheel_delta_x_ -= delta_x;
        wheel_delta_y_ -= delta_y;
        protocol::MouseEvent mouse_event;
        mouse_event.set_wheel_delta_x(delta_x);
        mouse_event.set_wheel_delta_y(delta_y);

        // Always include the ticks in the event, even if insufficient pixel
        // scrolling has accumulated for a single tick. This informs hosts
        // that can't inject pixel-based scroll events that the client will
        // accumulate them into tick-based scrolling, which gives a better
        // overall experience than trying to do this host-side.
        int ticks_x = static_cast<int>(wheel_ticks_x_);
        int ticks_y = static_cast<int>(wheel_ticks_y_);
        wheel_ticks_x_ -= ticks_x;
        wheel_ticks_y_ -= ticks_y;
        mouse_event.set_wheel_ticks_x(ticks_x);
        mouse_event.set_wheel_ticks_y(ticks_y);

        input_tracker_->InjectMouseEvent(mouse_event);
      }
      return true;
    }

    case PP_INPUTEVENT_TYPE_CHAR:
      // Consume but ignore character input events.
      return true;

    default: {
      VLOG(0) << "Unhandled input event: " << event.GetType();
      break;
    }
  }

  return false;
}

void PepperInputHandler::DidChangeFocus(bool has_focus) {
  has_focus_ = has_focus;
}

void PepperInputHandler::ReleaseAllIfModifiersStuck(
    const pp::InputEvent& event) {
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
    case PP_INPUTEVENT_TYPE_MOUSELEAVE:
      // Don't check modifiers on every mouse move event.
      break;

    case PP_INPUTEVENT_TYPE_KEYUP:
      // PPAPI doesn't always set modifiers correctly on KEYUP events. See
      // crbug.com/464791 for details.
      break;

    default: {
      uint32_t modifiers = event.GetModifiers();
      input_tracker_->ReleaseAllIfModifiersStuck(
          (modifiers & PP_INPUTEVENT_MODIFIER_ALTKEY) != 0,
          (modifiers & PP_INPUTEVENT_MODIFIER_CONTROLKEY) != 0,
          (modifiers & PP_INPUTEVENT_MODIFIER_METAKEY) != 0,
          (modifiers & PP_INPUTEVENT_MODIFIER_SHIFTKEY) != 0);
    }
  }
}

}  // namespace remoting
