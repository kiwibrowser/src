// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/settings_provider_gtk3.h"

#include "base/strings/string_split.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"

namespace libgtkui {

namespace {

const char kDefaultGtkLayout[] = "menu:minimize,maximize,close";

std::string GetGtkSettingsStringProperty(GtkSettings* settings,
                                         const gchar* prop_name) {
  GValue layout = G_VALUE_INIT;
  g_value_init(&layout, G_TYPE_STRING);
  g_object_get_property(G_OBJECT(settings), prop_name, &layout);
  DCHECK(G_VALUE_HOLDS_STRING(&layout));
  std::string prop_value(g_value_get_string(&layout));
  g_value_unset(&layout);
  return prop_value;
}

std::string GetDecorationLayoutFromGtkWindow() {
  static ScopedStyleContext context;
  if (!context) {
    context = GetStyleContextFromCss("");
    gtk_style_context_add_class(context, "csd");
  }

  gchar* layout_c = nullptr;
  gtk_style_context_get_style(context, "decoration-button-layout", &layout_c,
                              nullptr);
  DCHECK(layout_c);
  std::string layout(layout_c);
  g_free(layout_c);
  return layout;
}

void ParseActionString(const std::string& value,
                       GtkUi::NonClientWindowFrameAction* action) {
  if (value == "none")
    *action = views::LinuxUI::WINDOW_FRAME_ACTION_NONE;
  else if (value == "lower")
    *action = views::LinuxUI::WINDOW_FRAME_ACTION_LOWER;
  else if (value == "minimize")
    *action = views::LinuxUI::WINDOW_FRAME_ACTION_MINIMIZE;
  else if (value == "toggle-maximize")
    *action = views::LinuxUI::WINDOW_FRAME_ACTION_TOGGLE_MAXIMIZE;
  else if (value == "menu")
    *action = views::LinuxUI::WINDOW_FRAME_ACTION_MENU;
}

}  // namespace

SettingsProviderGtk3::FrameActionSettingWatcher::FrameActionSettingWatcher(
    SettingsProviderGtk3* settings_provider,
    const std::string& setting_name,
    views::LinuxUI::NonClientWindowFrameActionSourceType action_type,
    views::LinuxUI::NonClientWindowFrameAction default_action)
    : settings_provider_(settings_provider),
      setting_name_(setting_name),
      action_type_(action_type),
      default_action_(default_action) {
  GtkSettings* settings = gtk_settings_get_default();
  std::string notify_setting = "notify::" + setting_name;
  signal_id_ = g_signal_connect(settings, notify_setting.c_str(),
                                G_CALLBACK(OnSettingChangedThunk), this);
  DCHECK(signal_id_);
  OnSettingChanged(settings, nullptr);
}

SettingsProviderGtk3::FrameActionSettingWatcher::~FrameActionSettingWatcher() {
  if (signal_id_)
    g_signal_handler_disconnect(gtk_settings_get_default(), signal_id_);
}

void SettingsProviderGtk3::FrameActionSettingWatcher::OnSettingChanged(
    GtkSettings* settings,
    GParamSpec* param) {
  std::string value =
      GetGtkSettingsStringProperty(settings, setting_name_.c_str());
  GtkUi::NonClientWindowFrameAction action = default_action_;
  ParseActionString(value, &action);
  settings_provider_->delegate_->SetNonClientWindowFrameAction(action_type_,
                                                               action);
}

SettingsProviderGtk3::SettingsProviderGtk3(GtkUi* delegate)
    : delegate_(delegate), signal_id_decoration_layout_(0) {
  DCHECK(delegate_);
  GtkSettings* settings = gtk_settings_get_default();
  if (GtkVersionCheck(3, 14)) {
    signal_id_decoration_layout_ = g_signal_connect(
        settings, "notify::gtk-decoration-layout",
        G_CALLBACK(OnDecorationButtonLayoutChangedThunk), this);
    DCHECK(signal_id_decoration_layout_);
    OnDecorationButtonLayoutChanged(settings, nullptr);

    frame_action_setting_watchers_.push_back(
        std::make_unique<FrameActionSettingWatcher>(
            this, "gtk-titlebar-middle-click",
            views::LinuxUI::WINDOW_FRAME_ACTION_SOURCE_MIDDLE_CLICK,
            views::LinuxUI::WINDOW_FRAME_ACTION_NONE));
    frame_action_setting_watchers_.push_back(
        std::make_unique<FrameActionSettingWatcher>(
            this, "gtk-titlebar-double-click",
            views::LinuxUI::WINDOW_FRAME_ACTION_SOURCE_DOUBLE_CLICK,
            views::LinuxUI::WINDOW_FRAME_ACTION_TOGGLE_MAXIMIZE));
    frame_action_setting_watchers_.push_back(
        std::make_unique<FrameActionSettingWatcher>(
            this, "gtk-titlebar-right-click",
            views::LinuxUI::WINDOW_FRAME_ACTION_SOURCE_RIGHT_CLICK,
            views::LinuxUI::WINDOW_FRAME_ACTION_MENU));
  } else if (GtkVersionCheck(3, 10, 3)) {
    signal_id_decoration_layout_ =
        g_signal_connect_after(settings, "notify::gtk-theme-name",
                               G_CALLBACK(OnThemeChangedThunk), this);
    DCHECK(signal_id_decoration_layout_);
    OnThemeChanged(settings, nullptr);
  } else {
    // On versions older than 3.10.3, the layout was hardcoded.
    SetWindowButtonOrderingFromGtkLayout(kDefaultGtkLayout);
  }
}

SettingsProviderGtk3::~SettingsProviderGtk3() {
  if (signal_id_decoration_layout_) {
    g_signal_handler_disconnect(gtk_settings_get_default(),
                                signal_id_decoration_layout_);
  }
}

void SettingsProviderGtk3::SetWindowButtonOrderingFromGtkLayout(
    const std::string& gtk_layout) {
  std::vector<views::FrameButton> leading_buttons;
  std::vector<views::FrameButton> trailing_buttons;
  ParseButtonLayout(gtk_layout, &leading_buttons, &trailing_buttons);
  delegate_->SetWindowButtonOrdering(leading_buttons, trailing_buttons);
}

void SettingsProviderGtk3::OnDecorationButtonLayoutChanged(
    GtkSettings* settings,
    GParamSpec* param) {
  SetWindowButtonOrderingFromGtkLayout(
      GetGtkSettingsStringProperty(settings, "gtk-decoration-layout"));
}

void SettingsProviderGtk3::OnThemeChanged(GtkSettings* settings,
                                          GParamSpec* param) {
  std::string layout = GetDecorationLayoutFromGtkWindow();
  SetWindowButtonOrderingFromGtkLayout(layout);
}

}  // namespace libgtkui
