// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/window/dialog_delegate.h"

class Profile;

namespace views {
class Checkbox;
class Link;
}

class FirstRunDialog : public views::DialogDelegateView,
                       public views::LinkListener {
 public:
  // Displays the first run UI for reporting opt-in, import data etc.
  static void Show(Profile* profile);

 private:
  explicit FirstRunDialog(Profile* profile);
  ~FirstRunDialog() override;

  // This terminates the nested message-loop.
  void Done();

  // views::DialogDelegate:
  views::View* CreateExtraView() override;
  bool Accept() override;
  int GetDialogButtons() const override;

  // views::WidgetDelegate:
  void WindowClosing() override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  Profile* profile_;
  views::Checkbox* make_default_;
  views::Checkbox* report_crashes_;
  base::Closure quit_runloop_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunDialog);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_H_
