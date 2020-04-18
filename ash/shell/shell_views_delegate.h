// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_SHELL_VIEWS_DELEGATE_H_
#define ASH_SHELL_SHELL_VIEWS_DELEGATE_H_

#include "ui/views/test/test_views_delegate.h"

namespace ash {
namespace shell {

class ShellViewsDelegate : public views::TestViewsDelegate {
 public:
  ShellViewsDelegate();
  ~ShellViewsDelegate() override;

  // views::TestViewsDelegate:
  views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget) override;
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellViewsDelegate);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_SHELL_VIEWS_DELEGATE_H_
