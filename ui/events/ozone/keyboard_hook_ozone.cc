// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/keyboard_hook_base.h"

#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

namespace {

// A default implementation for Ozone platform.
class KeyboardHookOzone : public KeyboardHookBase {
 public:
  KeyboardHookOzone(base::Optional<base::flat_set<DomCode>> dom_codes,
                    KeyEventCallback callback);
  ~KeyboardHookOzone() override;

  bool Register();

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardHookOzone);
};

KeyboardHookOzone::KeyboardHookOzone(
    base::Optional<base::flat_set<DomCode>> dom_codes,
    KeyEventCallback callback)
    : KeyboardHookBase(std::move(dom_codes), std::move(callback)) {}

KeyboardHookOzone::~KeyboardHookOzone() = default;

bool KeyboardHookOzone::Register() {
  // TODO(680809): Implement system-level keyboard lock feature for ozone.
  // Return true to enable browser-level keyboard lock for ozone platform.
  return true;
}

}  // namespace

// static
std::unique_ptr<KeyboardHook> KeyboardHook::Create(
    base::Optional<base::flat_set<DomCode>> dom_codes,
    gfx::AcceleratedWidget accelerated_widget,
    KeyEventCallback callback) {
  std::unique_ptr<KeyboardHookOzone> keyboard_hook =
      std::make_unique<KeyboardHookOzone>(std::move(dom_codes),
                                          std::move(callback));

  if (!keyboard_hook->Register())
    return nullptr;

  return keyboard_hook;
}

}  // namespace ui
