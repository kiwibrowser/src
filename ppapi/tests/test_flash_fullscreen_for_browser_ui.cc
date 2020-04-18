// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash_fullscreen_for_browser_ui.h"

#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(FlashFullscreenForBrowserUI);

TestFlashFullscreenForBrowserUI::
    TestFlashFullscreenForBrowserUI(TestingInstance* instance)
    : TestCase(instance),
      screen_mode_(instance),
      view_change_event_(instance->pp_instance()),
      num_trigger_events_(0),
      request_fullscreen_(false),
      callback_factory_(this) {
  // This plugin should not be removed after this TestCase passes because
  // browser UI testing requires it to remain and to be interactive.
  instance_->set_remove_plugin(false);
}

TestFlashFullscreenForBrowserUI::~TestFlashFullscreenForBrowserUI() {
}

bool TestFlashFullscreenForBrowserUI::Init() {
  return CheckTestingInterface();
}

void TestFlashFullscreenForBrowserUI::RunTests(const std::string& filter) {
  RUN_TEST(EnterFullscreen, filter);
}

std::string TestFlashFullscreenForBrowserUI::TestEnterFullscreen() {
  if (screen_mode_.IsFullscreen())
    return ReportError("IsFullscreen() at start", true);

  // This is only allowed within a contet of a user gesture (e.g. mouse click).
  if (screen_mode_.SetFullscreen(true))
    return ReportError("SetFullscreen(true) outside of user gesture", true);

  // Trigger another call to SetFullscreen(true) from HandleInputEvent().
  // The transition is asynchronous and ends at the next DidChangeView().
  view_change_event_.Reset();
  request_fullscreen_ = true;
  instance_->RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
  SimulateUserGesture();
  // DidChangeView() will call the callback once in fullscreen mode.
  view_change_event_.Wait();
  if (GotError())
    return Error();

  if (!screen_mode_.IsFullscreen())
    return ReportError("IsFullscreen() in fullscreen", false);

  compositor_ = pp::Compositor(instance_);
  instance_->BindGraphics(compositor_);
  color_layer_ = compositor_.AddLayer();

  const int32_t result =
      instance_->RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                                             PP_INPUTEVENT_CLASS_KEYBOARD);
  if (result != PP_OK)
    return ReportError("RequestFilteringInputEvents() failed", result);

  PASS();
}

void TestFlashFullscreenForBrowserUI::DidChangeView(const pp::View& view) {
  layer_size_ = view.GetRect().size();
  if (normal_position_.IsEmpty())
    normal_position_ = view.GetRect();

  if (!compositor_.is_null())
    Paint(PP_OK);

  view_change_event_.Signal();
}

void TestFlashFullscreenForBrowserUI::SimulateUserGesture() {
  pp::Point plugin_center(
      normal_position_.x() + normal_position_.width() / 2,
      normal_position_.y() + normal_position_.height() / 2);
  pp::Point mouse_movement;
  pp::MouseInputEvent input_event(
      instance_,
      PP_INPUTEVENT_TYPE_MOUSEDOWN,
      NowInTimeTicks(),  // time_stamp
      0,  // modifiers
      PP_INPUTEVENT_MOUSEBUTTON_LEFT,
      plugin_center,
      1,  // click_count
      mouse_movement);

  testing_interface_->SimulateInputEvent(instance_->pp_instance(),
                                         input_event.pp_resource());
}

bool TestFlashFullscreenForBrowserUI::GotError() {
  return !error_.empty();
}

std::string TestFlashFullscreenForBrowserUI::Error() {
  std::string last_error = error_;
  error_.clear();
  return last_error;
}

void TestFlashFullscreenForBrowserUI::FailFullscreenTest(
    const std::string& error) {
  error_ = error;
  view_change_event_.Signal();
}

bool TestFlashFullscreenForBrowserUI::HandleInputEvent(
    const pp::InputEvent& event) {
  if (event.GetType() != PP_INPUTEVENT_TYPE_MOUSEDOWN &&
      event.GetType() != PP_INPUTEVENT_TYPE_MOUSEUP &&
      event.GetType() != PP_INPUTEVENT_TYPE_CHAR) {
    return false;
  }

  if (request_fullscreen_) {
    instance_->ClearInputEventRequest(PP_INPUTEVENT_CLASS_MOUSE);
    if (screen_mode_.IsFullscreen()) {
      FailFullscreenTest(
          ReportError("IsFullscreen() before fullscreen transition", true));
      return false;
    }
    request_fullscreen_ = false;
    if (!screen_mode_.SetFullscreen(true)) {
      FailFullscreenTest(
          ReportError("SetFullscreen(true) in normal", false));
      return false;
    }
    // DidChangeView() will complete the transition to fullscreen.
    return false;
  }

  ++num_trigger_events_;

  return true;
}

void TestFlashFullscreenForBrowserUI::Paint(int32_t last_compositor_result) {
  if (num_trigger_events_ == 0)
    color_layer_.SetColor(0.0f, 1.0f, 0.0f, 1.0f, layer_size_);
  else if (num_trigger_events_ % 2)
    color_layer_.SetColor(1.0f, 0.0f, 0.0f, 1.0f, layer_size_);
  else
    color_layer_.SetColor(0.0f, 0.0f, 1.0f, 1.0f, layer_size_);

  compositor_.CommitLayers(
      callback_factory_.NewCallback(&TestFlashFullscreenForBrowserUI::Paint));
}
