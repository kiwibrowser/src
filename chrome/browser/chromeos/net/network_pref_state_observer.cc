// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/network_pref_state_observer.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/network_handler.h"
#include "content/public/browser/notification_service.h"

namespace chromeos {

NetworkPrefStateObserver::NetworkPrefStateObserver() {
  // Initialize NetworkHandler with device prefs only.
  InitializeNetworkPrefServices(nullptr /* profile */);

  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                              content::NotificationService::AllSources());
}

NetworkPrefStateObserver::~NetworkPrefStateObserver() {
  NetworkHandler::Get()->ShutdownPrefServices();
}

void NetworkPrefStateObserver::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED, type);
  Profile* profile = content::Details<Profile>(details).ptr();
  DCHECK(profile);
  InitializeNetworkPrefServices(profile);
}

void NetworkPrefStateObserver::InitializeNetworkPrefServices(Profile* profile) {
  DCHECK(g_browser_process);
  NetworkHandler::Get()->InitializePrefServices(
      profile ? profile->GetPrefs() : nullptr,
      g_browser_process->local_state());
}

}  // namespace chromeos
