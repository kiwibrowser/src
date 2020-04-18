// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.h"

#include "base/run_loop.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/theme_profile_key.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/native_theme/native_theme_dark_aura.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/desktop_aura/x11_desktop_handler.h"
#include "ui/views/widget/native_widget_aura.h"

namespace {

ui::NativeTheme* GetNativeThemeForWindow(aura::Window* window) {
  if (!window)
    return nullptr;

  Profile* profile = GetThemeProfileForWindow(window);

  // If using the system (GTK) theme, don't use an Aura NativeTheme at all.
  // NB: ThemeService::UsingSystemTheme() might lag behind this pref. See
  // http://crbug.com/585522
  if (!profile || (!profile->IsSupervised() &&
                   profile->GetPrefs()->GetBoolean(prefs::kUsesSystemTheme))) {
    return nullptr;
  }

  // Use a dark theme for incognito browser windows that aren't
  // custom-themed. Otherwise, normal Aura theme.
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE &&
      ThemeServiceFactory::GetForProfile(profile)->UsingDefaultTheme() &&
      BrowserView::GetBrowserViewForNativeWindow(window)) {
    return ui::NativeThemeDarkAura::instance();
  }

  return ui::NativeTheme::GetInstanceForNativeUi();
}

}  // namespace

ChromeBrowserMainExtraPartsViewsLinux::ChromeBrowserMainExtraPartsViewsLinux() {
}

ChromeBrowserMainExtraPartsViewsLinux::
    ~ChromeBrowserMainExtraPartsViewsLinux() {
  if (views::X11DesktopHandler::get_dont_create())
    views::X11DesktopHandler::get_dont_create()->RemoveObserver(this);
}

void ChromeBrowserMainExtraPartsViewsLinux::PreEarlyInitialization() {
  // TODO(erg): Refactor this into a dlopen call when we add a GTK3 port.
  views::LinuxUI* gtk2_ui = BuildGtkUi();
  gtk2_ui->SetNativeThemeOverride(base::Bind(&GetNativeThemeForWindow));
  views::LinuxUI::SetInstance(gtk2_ui);
}

void ChromeBrowserMainExtraPartsViewsLinux::ToolkitInitialized() {
  ChromeBrowserMainExtraPartsViews::ToolkitInitialized();
  views::LinuxUI::instance()->Initialize();
}

void ChromeBrowserMainExtraPartsViewsLinux::PreCreateThreads() {
  // Update the device scale factor before initializing views
  // because its display::Screen instance depends on it.
  views::LinuxUI::instance()->UpdateDeviceScaleFactor();
  ChromeBrowserMainExtraPartsViews::PreCreateThreads();
  views::X11DesktopHandler::get()->AddObserver(this);
}

void ChromeBrowserMainExtraPartsViewsLinux::OnWorkspaceChanged(
    const std::string& new_workspace) {
  BrowserList::MoveBrowsersInWorkspaceToFront(new_workspace);
}
