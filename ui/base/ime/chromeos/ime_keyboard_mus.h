// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_CHROMEOS_IME_KEYBOARD_MUS_H_
#define UI_BASE_IME_CHROMEOS_IME_KEYBOARD_MUS_H_

#include "base/macros.h"
#include "ui/base/ime/chromeos/ime_keyboard.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {
class InputDeviceControllerClient;
}

namespace chromeos {
namespace input_method {

class UI_BASE_IME_EXPORT ImeKeyboardMus : public ImeKeyboard {
 public:
  explicit ImeKeyboardMus(
      ui::InputDeviceControllerClient* input_device_controller_client);
  ~ImeKeyboardMus() override;

  // ImeKeyboard:
  bool SetCurrentKeyboardLayoutByName(const std::string& layout_name) override;
  bool SetAutoRepeatRate(const AutoRepeatRate& rate) override;
  bool SetAutoRepeatEnabled(bool enabled) override;
  bool GetAutoRepeatEnabled() override;
  bool ReapplyCurrentKeyboardLayout() override;
  void ReapplyCurrentModifierLockStatus() override;
  void DisableNumLock() override;
  void SetCapsLockEnabled(bool enable_caps_lock) override;
  bool CapsLockIsEnabled() override;

 private:
  ui::InputDeviceControllerClient* input_device_controller_client_;

  DISALLOW_COPY_AND_ASSIGN(ImeKeyboardMus);
};

}  // namespace input_method
}  // namespace chromeos

#endif  // UI_BASE_IME_CHROMEOS_IME_KEYBOARD_MUS_H_
