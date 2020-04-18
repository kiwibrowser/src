// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/label_button_label.h"

namespace views {

LabelButtonLabel::LabelButtonLabel(const base::string16& text, int text_context)
    : Label(text, text_context, style::STYLE_PRIMARY) {}

LabelButtonLabel::~LabelButtonLabel() {}

void LabelButtonLabel::SetDisabledColor(SkColor color) {
  requested_disabled_color_ = color;
  disabled_color_set_ = true;
  if (!enabled())
    Label::SetEnabledColor(color);
}

void LabelButtonLabel::SetEnabledColor(SkColor color) {
  requested_enabled_color_ = color;
  enabled_color_set_ = true;
  if (enabled())
    Label::SetEnabledColor(color);
}

void LabelButtonLabel::OnEnabledChanged() {
  SetColorForEnableState();
  Label::OnEnabledChanged();
}

void LabelButtonLabel::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  SetColorForEnableState();
  Label::OnNativeThemeChanged(theme);
}

void LabelButtonLabel::SetColorForEnableState() {
  if (enabled() ? enabled_color_set_ : disabled_color_set_) {
    Label::SetEnabledColor(enabled() ? requested_enabled_color_
                                     : requested_disabled_color_);
  } else {
    int style = enabled() ? style::STYLE_PRIMARY : style::STYLE_DISABLED;
    Label::SetEnabledColor(style::GetColor(*this, text_context(), style));
  }
}

}  // namespace views
