// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_H_
#define UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"

namespace views {

class MenuButtonListener;

////////////////////////////////////////////////////////////////////////////////
//
// MenuButton
//
//  A button that shows a menu when the left mouse button is pushed
//
////////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT MenuButton : public LabelButton {
 public:
  // A scoped lock for keeping the MenuButton in STATE_PRESSED e.g., while a
  // menu is running. These are cumulative.
  class VIEWS_EXPORT PressedLock {
   public:
    explicit PressedLock(MenuButton* menu_button);
    // |event| is the event that caused the button to be pressed. May be null.
    PressedLock(MenuButton* menu_button,
                bool is_sibling_menu_show,
                const ui::LocatedEvent* event);
    ~PressedLock();

   private:
    base::WeakPtr<MenuButton> menu_button_;

    DISALLOW_COPY_AND_ASSIGN(PressedLock);
  };

  static const char kViewClassName[];

  // How much padding to put on the left and right of the menu marker.
  static const int kMenuMarkerPaddingLeft;
  static const int kMenuMarkerPaddingRight;

  // Create a Button.
  MenuButton(const base::string16& text,
             MenuButtonListener* menu_button_listener,
             bool show_menu_marker);
  ~MenuButton() override;

  bool show_menu_marker() const { return show_menu_marker_; }
  void set_menu_marker(const gfx::ImageSkia* menu_marker) {
    menu_marker_ = menu_marker;
  }
  const gfx::ImageSkia* menu_marker() const { return menu_marker_; }

  const gfx::Point& menu_offset() const { return menu_offset_; }
  void set_menu_offset(int x, int y) { menu_offset_.SetPoint(x, y); }

  // Activate the button (called when the button is pressed). |event| is the
  // event triggering the activation, if any.
  bool Activate(const ui::Event* event);

  // Returns true if the event is of the proper type to potentially trigger an
  // action. Since MenuButtons have properties other than event type (like
  // last menu open time) to determine if an event is valid to activate the
  // menu, this is distinct from IsTriggerableEvent().
  virtual bool IsTriggerableEventType(const ui::Event& event);

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  const char* GetClassName() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 protected:
  // Paint the menu marker image.
  void PaintMenuMarker(gfx::Canvas* canvas);

  // Overridden from LabelButton:
  gfx::Rect GetChildAreaBounds() override;

  // Overridden from Button:
  bool IsTriggerableEvent(const ui::Event& event) override;
  bool ShouldEnterPushedState(const ui::Event& event) override;
  void StateChanged(ButtonState old_state) override;
  void NotifyClick(const ui::Event& event) override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // Offset of the associated menu position.
  gfx::Point menu_offset_;

 private:
  friend class PressedLock;

  // Increment/decrement the number of "pressed" locks this button has, and
  // set the state accordingly. The ink drop is snapped to the final ACTIVATED
  // state if |snap_ink_drop_to_activated| is true, otherwise the ink drop will
  // be animated to the ACTIVATED node_data. The ink drop is animated at the
  // location of |event| if non-null, otherwise at the default location.
  void IncrementPressedLocked(bool snap_ink_drop_to_activated,
                              const ui::LocatedEvent* event);
  void DecrementPressedLocked();

  // Compute the maximum X coordinate for the current screen. MenuButtons
  // use this to make sure a menu is never shown off screen.
  int GetMaximumScreenXCoordinate();

  // We use a time object in order to keep track of when the menu was closed.
  // The time is used for simulating menu behavior for the menu button; that
  // is, if the menu is shown and the button is pressed, we need to close the
  // menu. There is no clean way to get the second click event because the
  // menu is displayed using a modal loop and, unlike regular menus in Windows,
  // the button is not part of the displayed menu.
  base::TimeTicks menu_closed_time_;

  // Our listener. Not owned.
  MenuButtonListener* listener_;

  // Whether or not we're showing a drop marker.
  bool show_menu_marker_;

  // The down arrow used to differentiate the menu button from normal buttons.
  const gfx::ImageSkia* menu_marker_;

  // The current number of "pressed" locks this button has.
  int pressed_lock_count_ = 0;

  // Used to let Activate() know if IncrementPressedLocked() was called.
  bool* increment_pressed_lock_called_ = nullptr;

  // True if the button was in a disabled state when a menu was run, and should
  // return to it once the press is complete. This can happen if, e.g., we
  // programmatically show a menu on a disabled button.
  bool should_disable_after_press_ = false;

  base::WeakPtrFactory<MenuButton> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MenuButton);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BUTTON_MENU_BUTTON_H_
