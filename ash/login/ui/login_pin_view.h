// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_PIN_VIEW_H_
#define ASH_LOGIN_UI_LOGIN_PIN_VIEW_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/login/ui/non_accessible_view.h"
#include "base/callback.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace base {
class Timer;
}  // namespace base

namespace ash {

// Implements a PIN keyboard. The class emits high-level events that can be used
// by the embedder. The PIN keyboard, while displaying letters, only emits
// numbers.
//
// The view is always rendered via layers.
//
// The UI looks a little like this:
//    _______    _______    _______
//   |   1   |  |   2   |  |   3   |
//   |       |  | A B C |  | D E F |
//    -------    -------    -------
//    _______    _______    _______
//   |   4   |  |   5   |  |   6   |
//   | G H I |  | J K L |  | M N O |
//    -------    -------    -------
//    _______    _______    _______
//   |   7   |  |   8   |  |   9   |
//   |P Q R S|  | T U V |  |W X Y Z|
//    -------    -------    -------
//               _______    _______
//              |   0   |  |  <-   |
//              |   +   |  |       |
//               -------    -------
//
class ASH_EXPORT LoginPinView : public NonAccessibleView {
 public:
  // Size of each button.
  static const int kButtonSizeDp;

  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(LoginPinView* view);
    ~TestApi();

    views::View* GetButton(int number) const;
    views::View* GetBackspaceButton() const;
    // Sets the timers that are used for backspace auto-submit. |delay_timer| is
    // the initial delay before an auto-submit, and |repeat_timer| fires
    // whenever a new backspace event should run after the initial delay.
    void SetBackspaceTimers(std::unique_ptr<base::Timer> delay_timer,
                            std::unique_ptr<base::Timer> repeat_timer);

   private:
    LoginPinView* const view_;
  };

  using OnPinKey = base::RepeatingCallback<void(int value)>;
  using OnPinBackspace = base::RepeatingClosure;

  // |on_key| is called whenever the user taps one of the pin buttons.
  // |on_backspace| is called when the user wants to erase the most recently
  // tapped key. Neither callback can be null.
  explicit LoginPinView(const OnPinKey& on_key,
                        const OnPinBackspace& on_backspace);
  ~LoginPinView() override;

  // Called when the password field text changed.
  void OnPasswordTextChanged(bool is_empty);

 private:
  class BackspacePinButton;

  BackspacePinButton* backspace_;
  OnPinKey on_key_;
  OnPinBackspace on_backspace_;

  DISALLOW_COPY_AND_ASSIGN(LoginPinView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_PIN_VIEW_H_
