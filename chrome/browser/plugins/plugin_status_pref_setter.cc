// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_status_pref_setter.h"

#include "base/bind.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/pepper_flash_settings_manager.h"
#include "chrome/browser/plugins/plugin_data_remover_helper.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/common/webplugininfo.h"

using content::BrowserThread;
using content::PluginService;

PluginStatusPrefSetter::PluginStatusPrefSetter()
    : profile_(NULL),
      factory_(this) {}

PluginStatusPrefSetter::~PluginStatusPrefSetter() {
}

void PluginStatusPrefSetter::Init(
    Profile* profile,
    const BooleanPrefMember::NamedChangeCallback& observer) {
  clear_plugin_lso_data_enabled_.Init(prefs::kClearPluginLSODataEnabled,
                                      profile->GetPrefs(), observer);
  pepper_flash_settings_enabled_.Init(prefs::kPepperFlashSettingsEnabled,
                                      profile->GetPrefs(), observer);
  profile_ = profile;
  registrar_.Add(this, chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED,
                 content::Source<Profile>(profile));
  StartUpdate();
}

void PluginStatusPrefSetter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED, type);
  StartUpdate();
}

void PluginStatusPrefSetter::StartUpdate() {
  PluginService::GetInstance()->GetPlugins(
      base::BindOnce(&PluginStatusPrefSetter::GotPlugins, factory_.GetWeakPtr(),
                     PluginPrefs::GetForProfile(profile_)));
}

void PluginStatusPrefSetter::GotPlugins(
    scoped_refptr<PluginPrefs> plugin_prefs,
    const std::vector<content::WebPluginInfo>& plugins) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Set the values on the PrefService instead of through the PrefMembers to
  // notify observers if they changed.
  profile_->GetPrefs()->SetBoolean(
      clear_plugin_lso_data_enabled_.GetPrefName().c_str(),
      PluginDataRemoverHelper::IsSupported(plugin_prefs.get()));
  profile_->GetPrefs()->SetBoolean(
      pepper_flash_settings_enabled_.GetPrefName().c_str(),
      PepperFlashSettingsManager::IsPepperFlashInUse(plugin_prefs.get(), NULL));
}
