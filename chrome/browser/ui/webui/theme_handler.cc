// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/theme_handler.h"

#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_ui.h"

///////////////////////////////////////////////////////////////////////////////
// ThemeHandler

ThemeHandler::ThemeHandler() {
}

ThemeHandler::~ThemeHandler() {
}

void ThemeHandler::RegisterMessages() {
  // These are not actual message registrations, but can't be done in the
  // constructor since they need the web_ui value to be set, which is done
  // post-construction, but before registering messages.
  InitializeCSSCaches();
  // Listen for theme installation.
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                 content::Source<ThemeService>(
                     ThemeServiceFactory::GetForProfile(GetProfile())));
}

void ThemeHandler::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_BROWSER_THEME_CHANGED, type);
  InitializeCSSCaches();
  bool has_custom_bg = ThemeService::GetThemeProviderForProfile(GetProfile())
                           .HasCustomImage(IDR_THEME_NTP_BACKGROUND);
  // TODO(dbeam): why does this need to be a dictionary?
  base::DictionaryValue dictionary;
  dictionary.SetBoolean("hasCustomBackground", has_custom_bg);
  web_ui()->CallJavascriptFunctionUnsafe("ntp.themeChanged", dictionary);
}

void ThemeHandler::InitializeCSSCaches() {
  Profile* profile = GetProfile();
  ThemeSource* theme = new ThemeSource(profile);
  content::URLDataSource::Add(profile, theme);
}

Profile* ThemeHandler::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}
