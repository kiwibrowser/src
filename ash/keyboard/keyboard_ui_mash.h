// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_KEYBOARD_UI_MASH_H_
#define ASH_KEYBOARD_KEYBOARD_UI_MASH_H_

#include <stdint.h>

#include <memory>

#include "ash/keyboard/keyboard_ui.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/keyboard/keyboard.mojom.h"

namespace service_manager {
class Connector;
}
namespace display {
class Display;
}

namespace ash {

class KeyboardUIMash : public KeyboardUI,
                       public keyboard::mojom::KeyboardObserver {
 public:
  // |connector| may be null in tests.
  explicit KeyboardUIMash(service_manager::Connector* connector);
  ~KeyboardUIMash() override;

  static std::unique_ptr<KeyboardUI> Create(
      service_manager::Connector* connector);

  // KeyboardUI:
  void Hide() override;
  void ShowInDisplay(const display::Display& display) override;
  bool IsEnabled() override;

  // keyboard::mojom::KeyboardObserver:
  void OnKeyboardStateChanged(bool is_enabled,
                              bool is_visible,
                              uint64_t display_id,
                              const gfx::Rect& bounds) override;

 private:
  bool is_enabled_;

  // May be null during tests.
  keyboard::mojom::KeyboardPtr keyboard_;
  mojo::Binding<keyboard::mojom::KeyboardObserver> observer_binding_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardUIMash);
};

}  // namespace ash

#endif  // ASH_KEYBOARD_KEYBOARD_UI_MASH_H_
