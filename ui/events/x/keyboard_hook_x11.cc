// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/stl_util.h"
#include "base/threading/thread_checker.h"
#include "ui/events/event.h"
#include "ui/events/keyboard_hook_base.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"

namespace ui {

namespace {

// XGrabKey essentially requires the modifier mask to explicitly be specified.
// You can specify 'AnyModifier' however doing so means the call to XGrabKey
// will fail if that key has been grabbed with any combination of modifiers.
// A common practice is to call XGrabKey with each individual modifier mask to
// avoid that problem.
const uint32_t kModifierMasks[] = {0,         // No additional modifier.
                                   Mod2Mask,  // Num lock.
                                   LockMask,  // Caps lock.
                                   Mod5Mask,  // Scroll lock.
                                   Mod2Mask | LockMask,
                                   Mod2Mask | Mod5Mask,
                                   LockMask | Mod5Mask,
                                   Mod2Mask | LockMask | Mod5Mask};

// This is the set of keys to lock when the website requests that all keys be
// locked.
const DomCode kDomCodesForLockAllKeys[] = {
    DomCode::ESCAPE,        DomCode::CONTEXT_MENU, DomCode::CONTROL_LEFT,
    DomCode::SHIFT_LEFT,    DomCode::ALT_LEFT,     DomCode::META_LEFT,
    DomCode::CONTROL_RIGHT, DomCode::SHIFT_RIGHT,  DomCode::ALT_RIGHT,
    DomCode::META_RIGHT};

// A default implementation for the X11 platform.
class KeyboardHookX11 : public KeyboardHookBase {
 public:
  KeyboardHookX11(base::Optional<base::flat_set<DomCode>> dom_codes,
                  gfx::AcceleratedWidget accelerated_widget,
                  KeyEventCallback callback);
  ~KeyboardHookX11() override;

  void Register();

 private:
  static KeyboardHookX11* instance_;

  // Helper methods for setting up key event capture.
  void CaptureAllKeys();
  void CaptureSpecificKeys();
  void CaptureKeyForDomCode(DomCode dom_code);

  THREAD_CHECKER(thread_checker_);

  // The x11 default display and the owner's native window.
  XDisplay* const x_display_ = nullptr;
  const gfx::AcceleratedWidget x_window_ = gfx::kNullAcceleratedWidget;

  // Tracks the keys that were grabbed.
  std::vector<int> grabbed_keys_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardHookX11);
};

// static
KeyboardHookX11* KeyboardHookX11::instance_ = nullptr;

KeyboardHookX11::KeyboardHookX11(
    base::Optional<base::flat_set<DomCode>> dom_codes,
    gfx::AcceleratedWidget accelerated_widget,
    KeyEventCallback callback)
    : KeyboardHookBase(std::move(dom_codes), std::move(callback)),
      x_display_(gfx::GetXDisplay()),
      x_window_(accelerated_widget) {}

KeyboardHookX11::~KeyboardHookX11() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  DCHECK_EQ(instance_, this);
  instance_ = nullptr;

  // Use XUngrabKeys for each key that has been grabbed.  XUngrabKeyboard
  // purportedly releases all keys when called and would not require the nested
  // loops, however in practice the keys are not actually released.
  for (int native_key_code : grabbed_keys_) {
    for (uint32_t modifier : kModifierMasks) {
      XUngrabKey(x_display_, native_key_code, modifier, x_window_);
    }
  }
}

void KeyboardHookX11::Register() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Only one instance of this class can be registered at a time.
  DCHECK(!instance_);
  instance_ = this;

  if (dom_codes().has_value())
    CaptureSpecificKeys();
  else
    CaptureAllKeys();
}

void KeyboardHookX11::CaptureAllKeys() {
  // We could have used the XGrabKeyboard API here instead of calling XGrabKeys
  // on a hard-coded set of shortcut keys.  Calling XGrabKeyboard would make
  // this work much simpler, however it has side-effects which prevents its use.
  // An example side-effect is that it prevents the lock screen from starting as
  // the screensaver process also calls XGrabKeyboard but will receive an error
  // since it was already grabbed by the window with KeyboardLock.
  for (size_t i = 0; i < base::size(kDomCodesForLockAllKeys); i++) {
    CaptureKeyForDomCode(kDomCodesForLockAllKeys[i]);
  }
}

void KeyboardHookX11::CaptureSpecificKeys() {
  for (DomCode dom_code : dom_codes().value()) {
    CaptureKeyForDomCode(dom_code);
  }
}

void KeyboardHookX11::CaptureKeyForDomCode(DomCode dom_code) {
  int native_key_code = KeycodeConverter::DomCodeToNativeKeycode(dom_code);
  if (native_key_code == KeycodeConverter::InvalidNativeKeycode())
    return;

  for (uint32_t modifier : kModifierMasks) {
    // XGrabKey always returns 1 so we can't rely on the return value to
    // determine if the grab succeeded.  Errors are reported to the global
    // error handler for debugging purposes but are not used to judge success.
    XGrabKey(x_display_, native_key_code, modifier, x_window_,
             /*owner_events=*/x11::False,
             /*pointer_mode=*/GrabModeAsync,
             /*keyboard_mode=*/GrabModeAsync);
  }

  grabbed_keys_.push_back(native_key_code);
}

}  // namespace

// static
std::unique_ptr<KeyboardHook> KeyboardHook::Create(
    base::Optional<base::flat_set<DomCode>> dom_codes,
    gfx::AcceleratedWidget accelerated_widget,
    KeyboardHook::KeyEventCallback callback) {
  std::unique_ptr<KeyboardHookX11> keyboard_hook =
      std::make_unique<KeyboardHookX11>(
          std::move(dom_codes), accelerated_widget, std::move(callback));

  keyboard_hook->Register();

  return keyboard_hook;
}

}  // namespace ui
