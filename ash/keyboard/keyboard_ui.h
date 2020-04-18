// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_KEYBOARD_UI_H_
#define ASH_KEYBOARD_KEYBOARD_UI_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace display {
class Display;
}

namespace ash {

class KeyboardUIObserver;

// KeyboardUI wraps the appropriate keyboard ui depending upon whether ash is
// running in mus or non-mus.
class ASH_EXPORT KeyboardUI {
 public:
  virtual ~KeyboardUI();

  static std::unique_ptr<KeyboardUI> Create();

  virtual void ShowInDisplay(const display::Display& display) = 0;
  virtual void Hide() = 0;

  // Returns true if the keyboard is enabled.
  virtual bool IsEnabled() = 0;

  void AddObserver(KeyboardUIObserver* observer);
  void RemoveObserver(KeyboardUIObserver* observer);

  // Applist also queries this for bounds. If app list remains in ash then
  // we need to plumb bounds through here too.

 protected:
  KeyboardUI();

  base::ObserverList<KeyboardUIObserver>* observers() { return &observers_; }

 private:
  base::ObserverList<KeyboardUIObserver> observers_;
};

}  // namespace ash

#endif  // ASH_KEYBOARD_KEYBOARD_UI_H_
