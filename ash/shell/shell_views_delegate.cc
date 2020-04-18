// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/shell_views_delegate.h"

#include "ash/shell.h"

namespace ash {
namespace shell {

ShellViewsDelegate::ShellViewsDelegate() = default;

ShellViewsDelegate::~ShellViewsDelegate() = default;

views::NonClientFrameView* ShellViewsDelegate::CreateDefaultNonClientFrameView(
    views::Widget* widget) {
  return ash::Shell::Get()->CreateDefaultNonClientFrameView(widget);
}

void ShellViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  if (params->opacity == views::Widget::InitParams::INFER_OPACITY)
    params->opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;

  if (params->native_widget)
    return;

  if (!params->parent && !params->context && !params->child)
    params->context = Shell::GetPrimaryRootWindow();
}

}  // namespace shell
}  // namespace ash
