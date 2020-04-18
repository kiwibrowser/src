// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_ACCESSIBILITY_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_ACCESSIBILITY_H_

#include "base/macros.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

namespace chromeos {
namespace input_method {

// Accessibility is a class handling accessibility feedbacks.
class Accessibility
    : public InputMethodManager::Observer {
 public:
  explicit Accessibility(InputMethodManager* imm);
  ~Accessibility() override;

 private:
  // InputMethodManager::Observer implementation.
  void InputMethodChanged(InputMethodManager* imm,
                          Profile* profile,
                          bool show_message) override;
  InputMethodManager* imm_;

  DISALLOW_COPY_AND_ASSIGN(Accessibility);
};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_ACCESSIBILITY_H_
