// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_HEADER_VIEW_H_
#define UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_HEADER_VIEW_H_

#include "base/macros.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/views/controls/button/button.h"

namespace views {
class ImageView;
class Label;
}

namespace message_center {

class NotificationControlButtonsView;

class NotificationHeaderView : public views::Button {
 public:
  NotificationHeaderView(NotificationControlButtonsView* control_buttons_view,
                         views::ButtonListener* listener);
  void SetAppIcon(const gfx::ImageSkia& img);
  void SetAppName(const base::string16& name);
  void SetProgress(int progress);
  void SetOverflowIndicator(int count);
  void SetTimestamp(base::Time past);
  void SetExpandButtonEnabled(bool enabled);
  void SetExpanded(bool expanded);
  void SetSettingsButtonEnabled(bool enabled);
  void SetCloseButtonEnabled(bool enabled);
  void SetControlButtonsVisible(bool visible);
  // Set the unified theme color used among the app icon, app name, and expand
  // button.
  void SetAccentColor(SkColor color);
  void ClearAppIcon();
  void ClearProgress();
  void ClearOverflowIndicator();
  void ClearTimestamp();
  bool IsExpandButtonEnabled();
  void SetSubpixelRenderingEnabled(bool enabled);

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // Button override:
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;

  views::ImageView* expand_button() { return expand_button_; }

  SkColor accent_color_for_testing() { return accent_color_; }

 private:
  // Update visibility for both |summary_text_view_| and |timestamp_view_|.
  void UpdateSummaryTextVisibility();

  SkColor accent_color_ = kNotificationDefaultAccentColor;

  views::Label* app_name_view_ = nullptr;
  views::Label* summary_text_divider_ = nullptr;
  views::Label* summary_text_view_ = nullptr;
  views::Label* timestamp_divider_ = nullptr;
  views::Label* timestamp_view_ = nullptr;
  views::ImageView* app_icon_view_ = nullptr;
  views::ImageView* expand_button_ = nullptr;

  bool settings_button_enabled_ = false;
  bool has_progress_ = false;
  bool has_overflow_indicator_ = false;
  bool has_timestamp_ = false;
  bool is_expanded_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationHeaderView);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_VIEWS_NOTIFICATION_HEADER_VIEW_H_
