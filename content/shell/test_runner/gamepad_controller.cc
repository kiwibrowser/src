// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/gamepad_controller.h"

#include <string.h>

#include "base/macros.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "third_party/blink/public/platform/web_gamepad_listener.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

using device::Gamepad;
using device::Gamepads;

namespace test_runner {

class GamepadControllerBindings
    : public gin::Wrappable<GamepadControllerBindings> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static void Install(base::WeakPtr<GamepadController> controller,
                      blink::WebLocalFrame* frame);

 private:
  explicit GamepadControllerBindings(
      base::WeakPtr<GamepadController> controller);
  ~GamepadControllerBindings() override;

  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void Connect(int index);
  void DispatchConnected(int index);
  void Disconnect(int index);
  void SetId(int index, const std::string& src);
  void SetButtonCount(int index, int buttons);
  void SetButtonData(int index, int button, double data);
  void SetAxisCount(int index, int axes);
  void SetAxisData(int index, int axis, double data);
  void SetDualRumbleVibrationActuator(int index, bool enabled);

  base::WeakPtr<GamepadController> controller_;

  DISALLOW_COPY_AND_ASSIGN(GamepadControllerBindings);
};

gin::WrapperInfo GamepadControllerBindings::kWrapperInfo = {
    gin::kEmbedderNativeGin};

// static
void GamepadControllerBindings::Install(
    base::WeakPtr<GamepadController> controller,
    blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<GamepadControllerBindings> bindings =
      gin::CreateHandle(isolate, new GamepadControllerBindings(controller));
  if (bindings.IsEmpty())
    return;
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "gamepadController"), bindings.ToV8());
}

GamepadControllerBindings::GamepadControllerBindings(
    base::WeakPtr<GamepadController> controller)
    : controller_(controller) {}

GamepadControllerBindings::~GamepadControllerBindings() {}

gin::ObjectTemplateBuilder GamepadControllerBindings::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<GamepadControllerBindings>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("connect", &GamepadControllerBindings::Connect)
      .SetMethod("dispatchConnected",
                 &GamepadControllerBindings::DispatchConnected)
      .SetMethod("disconnect", &GamepadControllerBindings::Disconnect)
      .SetMethod("setId", &GamepadControllerBindings::SetId)
      .SetMethod("setButtonCount", &GamepadControllerBindings::SetButtonCount)
      .SetMethod("setButtonData", &GamepadControllerBindings::SetButtonData)
      .SetMethod("setAxisCount", &GamepadControllerBindings::SetAxisCount)
      .SetMethod("setAxisData", &GamepadControllerBindings::SetAxisData)
      .SetMethod("setDualRumbleVibrationActuator",
                 &GamepadControllerBindings::SetDualRumbleVibrationActuator);
}

void GamepadControllerBindings::Connect(int index) {
  if (controller_)
    controller_->Connect(index);
}

void GamepadControllerBindings::DispatchConnected(int index) {
  if (controller_)
    controller_->DispatchConnected(index);
}

void GamepadControllerBindings::Disconnect(int index) {
  if (controller_)
    controller_->Disconnect(index);
}

void GamepadControllerBindings::SetId(int index, const std::string& src) {
  if (controller_)
    controller_->SetId(index, src);
}

void GamepadControllerBindings::SetButtonCount(int index, int buttons) {
  if (controller_)
    controller_->SetButtonCount(index, buttons);
}

void GamepadControllerBindings::SetButtonData(int index,
                                              int button,
                                              double data) {
  if (controller_)
    controller_->SetButtonData(index, button, data);
}

void GamepadControllerBindings::SetAxisCount(int index, int axes) {
  if (controller_)
    controller_->SetAxisCount(index, axes);
}

void GamepadControllerBindings::SetAxisData(int index, int axis, double data) {
  if (controller_)
    controller_->SetAxisData(index, axis, data);
}

void GamepadControllerBindings::SetDualRumbleVibrationActuator(int index,
                                                               bool enabled) {
  if (controller_)
    controller_->SetDualRumbleVibrationActuator(index, enabled);
}

// static
base::WeakPtr<GamepadController> GamepadController::Create(
    WebTestDelegate* delegate) {
  CHECK(delegate);

  GamepadController* controller = new GamepadController();
  delegate->SetGamepadProvider(controller);
  return controller->weak_factory_.GetWeakPtr();
}

GamepadController::GamepadController()
    : listener_(nullptr), weak_factory_(this) {
  Reset();
}

GamepadController::~GamepadController() {}

void GamepadController::Reset() {
  memset(&gamepads_, 0, sizeof(gamepads_));
}

void GamepadController::Install(blink::WebLocalFrame* frame) {
  GamepadControllerBindings::Install(weak_factory_.GetWeakPtr(), frame);
}

void GamepadController::SampleGamepads(Gamepads& gamepads) {
  memcpy(&gamepads, &gamepads_, sizeof(Gamepads));
}

void GamepadController::SetListener(blink::WebGamepadListener* listener) {
  listener_ = listener;
}

void GamepadController::Connect(int index) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  gamepads_.items[index].connected = true;
}

void GamepadController::DispatchConnected(int index) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap) ||
      !gamepads_.items[index].connected)
    return;
  const Gamepad& pad = gamepads_.items[index];
  if (listener_)
    listener_->DidConnectGamepad(index, pad);
}

void GamepadController::Disconnect(int index) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  Gamepad& pad = gamepads_.items[index];
  pad.connected = false;
  if (listener_)
    listener_->DidDisconnectGamepad(index, pad);
}

void GamepadController::SetId(int index, const std::string& src) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  const char* p = src.c_str();
  memset(gamepads_.items[index].id, 0, sizeof(gamepads_.items[index].id));
  for (unsigned i = 0; *p && i < Gamepad::kIdLengthCap - 1; ++i)
    gamepads_.items[index].id[i] = *p++;
}

void GamepadController::SetButtonCount(int index, int buttons) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  if (buttons < 0 || buttons >= static_cast<int>(Gamepad::kButtonsLengthCap))
    return;
  gamepads_.items[index].buttons_length = buttons;
}

void GamepadController::SetButtonData(int index, int button, double data) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  if (button < 0 || button >= static_cast<int>(Gamepad::kButtonsLengthCap))
    return;
  gamepads_.items[index].buttons[button].value = data;
  gamepads_.items[index].buttons[button].pressed = data > 0.1f;
}

void GamepadController::SetAxisCount(int index, int axes) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  if (axes < 0 || axes >= static_cast<int>(Gamepad::kAxesLengthCap))
    return;
  gamepads_.items[index].axes_length = axes;
}

void GamepadController::SetAxisData(int index, int axis, double data) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  if (axis < 0 || axis >= static_cast<int>(Gamepad::kAxesLengthCap))
    return;
  gamepads_.items[index].axes[axis] = data;
}

void GamepadController::SetDualRumbleVibrationActuator(int index,
                                                       bool enabled) {
  if (index < 0 || index >= static_cast<int>(Gamepads::kItemsLengthCap))
    return;
  gamepads_.items[index].vibration_actuator.type =
      device::GamepadHapticActuatorType::kDualRumble;
  gamepads_.items[index].vibration_actuator.not_null = enabled;
}

}  // namespace test_runner
