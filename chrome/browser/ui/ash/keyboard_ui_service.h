// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_KEYBOARD_UI_SERVICE_H_
#define CHROME_BROWSER_UI_ASH_KEYBOARD_UI_SERVICE_H_

#include "ui/keyboard/keyboard.mojom.h"

class KeyboardUIService : public keyboard::mojom::Keyboard {
 public:
  KeyboardUIService();
  ~KeyboardUIService() override;

  // keyboard::mojom::Keyboard:
  void Show() override;
  void Hide() override;
  void AddObserver(keyboard::mojom::KeyboardObserverPtr observer) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardUIService);
};

#endif  // CHROME_BROWSER_UI_ASH_KEYBOARD_UI_SERVICE_H_
