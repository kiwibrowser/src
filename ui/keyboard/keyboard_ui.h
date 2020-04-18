// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_UI_H_
#define UI_KEYBOARD_KEYBOARD_UI_H_

#include "base/macros.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/keyboard/keyboard_export.h"

namespace aura {
class Window;
}
namespace gfx {
class Rect;
}
namespace ui {
class InputMethod;
}

namespace keyboard {

class KeyboardController;

// An interface implemented by an object that implements a keyboard UI.
class KEYBOARD_EXPORT KeyboardUI {
 public:
  KeyboardUI();
  virtual ~KeyboardUI();

  // Gets the virtual keyboard contents window i.e. the WebContents window where
  // keyboard extensions are loaded. May return null if the window has not yet
  // been created.
  virtual aura::Window* GetContentsWindow() = 0;

  // Whether the keyboard contents window has been created.
  virtual bool HasContentsWindow() const = 0;

  // Whether this window should do an overscroll to avoid occlusion by the
  // virtual keyboard. IME windows and virtual keyboard windows should always
  // avoid overscroll.
  virtual bool ShouldWindowOverscroll(aura::Window* window) const = 0;

  // Gets the InputMethod that will provide notifications about changes in the
  // text input context.
  virtual ui::InputMethod* GetInputMethod() = 0;

  // Shows the container window of the keyboard. The default implementation
  // simply shows the container. An overridden implementation can set up
  // necessary animation, or delay the visibility change as it desires.
  virtual void ShowKeyboardContainer(aura::Window* container);

  // Hides the container window of the keyboard. The default implementation
  // simply hides the container. An overridden implementation can set up
  // necesasry animation, or delay the visibility change as it desires.
  virtual void HideKeyboardContainer(aura::Window* container);

  // Ensures caret in current work area (not occluded by virtual keyboard
  // window).
  virtual void EnsureCaretInWorkArea(const gfx::Rect& occluded_bounds);

  // KeyboardController owns the KeyboardUI instance so KeyboardUI subclasses
  // should not take ownership of the |controller|. |controller| can be null
  // when KeyboardController is destroying.
  virtual void SetController(KeyboardController* controller);

  // Reloads virtual keyboard URL if the current keyboard's web content URL is
  // different. The URL can be different if user switch from password field to
  // any other type input field.
  // At password field, the system virtual keyboard is forced to load even if
  // the current IME provides a customized virtual keyboard. This is needed to
  // prevent IME virtual keyboard logging user's password. Once user switch to
  // other input fields, the virtual keyboard should switch back to the IME
  // provided keyboard, or keep using the system virtual keyboard if IME doesn't
  // provide one.
  virtual void ReloadKeyboardIfNeeded() = 0;

  // When the embedder changes the keyboard bounds, asks the keyboard to adjust
  // insets for windows affected by this.
  virtual void InitInsets(const gfx::Rect& keyboard_bounds) = 0;

  // Resets insets for affected windows.
  virtual void ResetInsets() = 0;

 protected:
  KeyboardController* keyboard_controller() { return keyboard_controller_; }

 private:
  keyboard::KeyboardController* keyboard_controller_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardUI);
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_UI_H_
