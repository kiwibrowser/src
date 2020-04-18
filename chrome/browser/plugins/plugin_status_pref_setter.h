// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PLUGIN_STATUS_PREF_SETTER_H_
#define CHROME_BROWSER_PLUGINS_PLUGIN_STATUS_PREF_SETTER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PluginPrefs;
class PrefService;
class Profile;

namespace content {
struct WebPluginInfo;
}

// Helper class modeled after BooleanPrefMember to (asynchronously) update
// preferences related to plugin enable status.
// It should only be used from the UI thread. The client has to make sure that
// the passed profile outlives this object.
class PluginStatusPrefSetter : public content::NotificationObserver {
 public:
  PluginStatusPrefSetter();
  ~PluginStatusPrefSetter() override;

  // Binds the preferences in the profile's PrefService, notifying |observer| if
  // any value changes.
  // This asynchronously calls the PluginService to get the list of installed
  // plugins.
  void Init(Profile* profile,
            const BooleanPrefMember::NamedChangeCallback& observer);

  bool IsClearPluginLSODataEnabled() const {
    return clear_plugin_lso_data_enabled_.GetValue();
  }

  bool IsPepperFlashSettingsEnabled() const {
    return pepper_flash_settings_enabled_.GetValue();
  }

  // content::NotificationObserver methods:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  void StartUpdate();
  void GotPlugins(scoped_refptr<PluginPrefs> plugin_prefs,
                  const std::vector<content::WebPluginInfo>& plugins);

  content::NotificationRegistrar registrar_;
  // Weak pointer.
  Profile* profile_;

  // Whether clearing LSO data is supported.
  BooleanPrefMember clear_plugin_lso_data_enabled_;
  // Whether we should show Pepper Flash-specific settings.
  BooleanPrefMember pepper_flash_settings_enabled_;

  base::WeakPtrFactory<PluginStatusPrefSetter> factory_;

  DISALLOW_COPY_AND_ASSIGN(PluginStatusPrefSetter);
};

#endif  // CHROME_BROWSER_PLUGINS_PLUGIN_STATUS_PREF_SETTER_H_
