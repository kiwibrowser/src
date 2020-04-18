// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_service_factory.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"

#if defined(OS_WIN)
#include "chrome/browser/themes/theme_service_win.h"
#elif defined(USE_X11)
#include "chrome/browser/themes/theme_service_aurax11.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

// static
ThemeService* ThemeServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<ThemeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
const extensions::Extension* ThemeServiceFactory::GetThemeForProfile(
    Profile* profile) {
  std::string id = GetForProfile(profile)->GetThemeID();
  if (id == ThemeService::kDefaultThemeID)
    return NULL;

  return extensions::ExtensionRegistry::Get(
      profile)->enabled_extensions().GetByID(id);
}

// static
ThemeServiceFactory* ThemeServiceFactory::GetInstance() {
  return base::Singleton<ThemeServiceFactory>::get();
}

ThemeServiceFactory::ThemeServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ThemeService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
}

ThemeServiceFactory::~ThemeServiceFactory() {}

KeyedService* ThemeServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  ThemeService* provider = NULL;
#if defined(OS_WIN)
  provider = new ThemeServiceWin;
#elif defined(USE_X11)
  provider = new ThemeServiceAuraX11;
#else
  provider = new ThemeService;
#endif
  provider->Init(static_cast<Profile*>(profile));

  return provider;
}

void ThemeServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if defined(USE_X11)
  bool default_uses_system_theme = false;

  const views::LinuxUI* linux_ui = views::LinuxUI::instance();
  if (linux_ui)
    default_uses_system_theme = linux_ui->GetDefaultUsesSystemTheme();

  registry->RegisterBooleanPref(prefs::kUsesSystemTheme,
                                default_uses_system_theme);
#endif
  registry->RegisterFilePathPref(prefs::kCurrentThemePackFilename,
                                 base::FilePath());
  registry->RegisterStringPref(prefs::kCurrentThemeID,
                               ThemeService::kDefaultThemeID);
  registry->RegisterDictionaryPref(prefs::kCurrentThemeImages);
  registry->RegisterDictionaryPref(prefs::kCurrentThemeColors);
  registry->RegisterDictionaryPref(prefs::kCurrentThemeTints);
  registry->RegisterDictionaryPref(prefs::kCurrentThemeDisplayProperties);
}

content::BrowserContext* ThemeServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool ThemeServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
