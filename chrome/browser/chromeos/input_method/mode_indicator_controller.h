// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "ui/base/ime/chromeos/input_method_manager.h"
#include "ui/chromeos/ime/mode_indicator_view.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget_observer.h"

namespace chromeos {
namespace input_method {

// Publicly visible for testing.
class ModeIndicatorObserverInterface : public views::WidgetObserver {
 public:
  ModeIndicatorObserverInterface() {}
  ~ModeIndicatorObserverInterface() override {}

  virtual void AddModeIndicatorWidget(views::Widget* widget) = 0;
};


// ModeIndicatorController is the controller of ModeIndicatiorDelegateView
// on the MVC model.
class ModeIndicatorController : public InputMethodManager::Observer,
                                public ui::ime::ModeIndicatorView::Delegate {
 public:
  explicit ModeIndicatorController(InputMethodManager* imm);
  ~ModeIndicatorController() override;

  // Set cursor bounds, which is the base point to display this indicator.
  // Bacisally this indicator is displayed underneath the cursor.
  void SetCursorBounds(const gfx::Rect& cursor_location);

  // Notify the focus state to the mode indicator.
  void FocusStateChanged(bool is_focused);

  // Accessor of the widget observer for testing.  The caller keeps the
  // ownership of the observer object.
  static void SetModeIndicatorObserverForTesting(
      ModeIndicatorObserverInterface* observer);

 private:
  // InputMethodManager::Observer implementation.
  void InputMethodChanged(InputMethodManager* manager,
                          Profile* profile,
                          bool show_message) override;

  // ui::ime::ModeIndicatorView::Delegate:
  void InitWidgetContainer(views::Widget::InitParams* params) override;

  // Show the mode inidicator with the current ime's short name if all
  // the conditions are cleared.
  void ShowModeIndicator();

  InputMethodManager* imm_;

  // Cursor bounds representing the anchor rect of the mode indicator.
  gfx::Rect cursor_bounds_;

  // True on a text field is focused.
  bool is_focused_;

  // Observer of the widgets created by BubbleDialogDelegateView.  This is used
  // to close the previous widget when a new widget is created.
  std::unique_ptr<ModeIndicatorObserverInterface> mi_observer_;

  DISALLOW_COPY_AND_ASSIGN(ModeIndicatorController);
};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_MODE_INDICATOR_CONTROLLER_H_
