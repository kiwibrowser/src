// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_DELEGATE_H_
#define CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_DELEGATE_H_

#include "base/macros.h"
#include "chrome/browser/vr/keyboard_delegate.h"
#include "chrome/browser/vr/keyboard_ui_interface.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "chrome/browser/vr/testapp/test_keyboard_renderer.h"
#include "ui/gfx/transform.h"

namespace gfx {
class Point3F;
}  // namespace gfx

namespace ui {
class Event;
}  // namespace ui

namespace vr {

class UiElementRenderer;
struct CameraModel;

class TestKeyboardDelegate : public KeyboardDelegate {
 public:
  TestKeyboardDelegate();
  ~TestKeyboardDelegate() override;

  void ShowKeyboard() override;
  void HideKeyboard() override;
  void SetTransform(const gfx::Transform& transform) override;
  bool HitTest(const gfx::Point3F& ray_origin,
               const gfx::Point3F& ray_target,
               gfx::Point3F* hit_position) override;
  void Draw(const CameraModel& model) override;
  bool SupportsSelection() override;

  void Initialize(SkiaSurfaceProvider* provider, UiElementRenderer* renderer);
  void SetUiInterface(KeyboardUiInterface* keyboard) {
    ui_interface_ = keyboard;
  }
  void UpdateInput(const vr::TextInputInfo& info);
  bool HandleInput(ui::Event* e);

 private:
  std::unique_ptr<TestKeyboardRenderer> renderer_;
  KeyboardUiInterface* ui_interface_ = nullptr;
  gfx::Transform world_space_transform_;
  bool editing_;
  TextInputInfo input_info_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TESTAPP_TEST_KEYBOARD_DELEGATE_H_
