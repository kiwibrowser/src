// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/theme_copying_widget.h"

ThemeCopyingWidget::ThemeCopyingWidget(views::Widget* role_model)
    : role_model_(role_model) {}

ThemeCopyingWidget::~ThemeCopyingWidget() {}

const ui::NativeTheme* ThemeCopyingWidget::GetNativeTheme() const {
  return role_model_->GetNativeTheme();
}
