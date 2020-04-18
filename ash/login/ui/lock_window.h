// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOCK_WINDOW_H_
#define ASH_LOGIN_UI_LOCK_WINDOW_H_

#include "ash/ash_export.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "base/macros.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class View;
class Widget;
}  // namespace views

namespace ash {

enum class Config;

// Shows the widget for the lock screen.
class ASH_EXPORT LockWindow : public views::Widget,
                              public views::WidgetDelegate {
 public:
  explicit LockWindow(Config config);
  ~LockWindow() override;

  LoginDataDispatcher* data_dispatcher() const {
    return data_dispatcher_.get();
  }

  // TODO(jdufault): After webui login is removed move |data_dispatcher| to a
  // constructor parameter and assert it is non-null.
  void set_data_dispatcher(
      std::unique_ptr<LoginDataDispatcher> data_dispatcher) {
    data_dispatcher_ = std::move(data_dispatcher);
  }

 private:
  // views::WidgetDelegate:
  views::View* GetInitiallyFocusedView() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  // |data_dispatcher_| is owned by LockWindow because it needs to live longer
  // than the attached view hierarchy.
  std::unique_ptr<LoginDataDispatcher> data_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(LockWindow);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOCK_WINDOW_H_
